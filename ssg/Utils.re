type jsError = Shared.Utils.jsError;

let makeError = Shared.Utils.makeError;

let getStack = Shared.Utils.getStack;

let getFilepathFromError = Shared.Utils.getFilepathFromError;

external window: _ = "window";

let getFilepath = () =>
  switch (Js.typeof(window) == "undefined") {
  // Get filepath only in node
  | false => ""
  | true => makeError()->getFilepathFromError
  };

let getDirname = Shared.Utils.getDirname;

let getModuleNameFromModulePath = modulePath => {
  let segments = modulePath->Js.String2.split("/");

  let filename =
    segments->Js.Array2.copy->Js.Array2.reverseInPlace->Belt.Array.get(0);

  switch (filename) {
  | None
  | Some("") =>
    Js.Console.error(
      "[Utils.getModuleNameFromModulePath] Filename is empty or None",
    );
    Process.exit(1);
  | Some(filename) => filename->Js.String2.replace(".bs.js", "")
  };
};
