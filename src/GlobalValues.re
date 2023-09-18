external globalThis: Js.Dict.t(string) = "globalThis";
external globalThisJson: Js.Dict.t(Js.Json.t) = "globalThis";

let unsafeAdd = (globalValues: array((string, string))) => {
  globalValues->Js.Array2.forEach(((key, value)) => {
    globalThis->Js.Dict.set(key, value)
  });
};

let unsafeAddJson = (globalValues: array((string, Js.Json.t))) => {
  globalValues->Js.Array2.forEach(((key, value)) => {
    globalThisJson->Js.Dict.set(key, value)
  });
};
