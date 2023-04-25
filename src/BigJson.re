// The same type as in Js.Json module but with an additional Unsupported variant.
// We need to handle fields with possible undefined or other unsupported values (JSON.stringify ignores them).
type tagged_t =
  | JSONFalse
  | JSONTrue
  | JSONNull
  | JSONString(string)
  | JSONNumber(float)
  | JSONObject(Js_dict.t(Js.Json.t))
  | JSONArray(array(Js.Json.t))
  | Unsupported;

let classify = (x: Js.Json.t): tagged_t => {
  let typeof = Js.typeof(x);
  if (typeof == "string") {
    JSONString(Obj.magic(x));
  } else if (typeof == "number") {
    JSONNumber(Obj.magic(x));
  } else if (typeof == "boolean") {
    if (Obj.magic(x) == true) {
      JSONTrue;
    } else {
      JSONFalse;
    };
  } else if (Obj.magic(x) === Js.null) {
    JSONNull;
  } else if (Js_array2.isArray(x)) {
    JSONArray(Obj.magic(x));
  } else if (typeof == "object") {
    JSONObject(Obj.magic(x));
  } else {
    Unsupported;
  };
};

let jsonStringify =
  (. json) => Js.Json.stringifyAny(json)->Belt.Option.getWithDefault("");

let rec stringifyRec =
  (. json: Js.Json.t, currentDepth: int, appendToFile: (. string) => unit) => {
    switch (classify(json)) {
    | Unsupported => ()
    | JSONFalse => appendToFile(. jsonStringify(. json))
    | JSONTrue => appendToFile(. jsonStringify(. json))
    | JSONNull => appendToFile(. jsonStringify(. json))
    | JSONString(_s) => appendToFile(. jsonStringify(. json))
    | JSONNumber(_n) => appendToFile(. jsonStringify(. json))
    | JSONObject(obj) =>
      // If we are at depth 2+ we stop traversing and use regular JSON.stringify which is faster
      if (currentDepth > 1) {
        appendToFile(. jsonStringify(. json));
      } else {
        stringifyObject(. obj, currentDepth, appendToFile);
      }
    | JSONArray(arr) =>
      if (currentDepth > 1) {
        appendToFile(. jsonStringify(. json));
      } else {
        stringifyJsonArray(. arr, currentDepth, appendToFile);
      }
    };
  }
and stringifyObject =
  (.
    dict: Js.Dict.t(Js.Json.t),
    currentDepth,
    appendToFile: (. string) => unit,
  ) => {
    appendToFile(. "{");

    let entries = dict->Js.Dict.entries;
    let length = entries->Js.Array.length;

    entries->Js.Array2.forEachi(((key, value), index) => {
      switch (classify(value)) {
      | Unsupported => ()
      | _ =>
        appendToFile(. {j|"$(key)":|j});

        stringifyRec(. value, currentDepth + 1, appendToFile);

        if (index == length - 1) {
          ();
        } else {
          appendToFile(. ",");
        };
      }
    });

    appendToFile(. "}");
  }
and stringifyJsonArray =
  (. array: array(Js.Json.t), currentDepth, appendToFile: (. string) => unit) => {
    appendToFile(. "[");

    let length = array->Js.Array.length;

    array->Js.Array2.forEachi((json, index) =>
      if (Js.typeof(json) == "undefined") {
        ();
      } else {
        stringifyRec(. json, currentDepth + 1, appendToFile);

        if (index == length - 1) {
          ();
        } else {
          appendToFile(. ",");
        };
      }
    );

    appendToFile(. "]");
  };

let stringifyToFile =
  (. json: Js.Json.t, outputFilepath: string) => {
    let () = Fs.rmSync(outputFilepath, {force: true, recursive: false});

    let appendStringToFile =
      (. data) =>
        Fs.appendFileSync(
          ~path=outputFilepath,
          ~data,
          ~options=Js.Obj.empty(),
        );

    let () = stringifyRec(. json, 0, appendStringToFile);
    ();
  };
