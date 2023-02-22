[@val] external globalThis: Js.Dict.t(string) = "globalThis";

let unsafeAdd = (globalValues: array((string, string))) => {
  globalValues->Js.Array2.forEach(((key, value)) => {
    globalThis->Js.Dict.set(key, value)
  });
};
