external globalThis: Js.Dict.t(string) = "globalThis";
external globalThisJson: Js.Dict.t(Js.Json.t) = "globalThis";

let unsafeAdd = (globalValues: array((string, string))) => {
  globalValues->Js.Array.forEach(~f=((key, value)) => {
    globalThis->Js.Dict.set(key, value)
  }, _);
};

let unsafeAddJson = (globalValues: array((string, Js.Json.t))) => {
  globalValues->Js.Array.forEach(~f=((key, value)) => {
    globalThisJson->Js.Dict.set(key, value)
  }, _);
};
