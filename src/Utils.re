type jsError;

[@new] external makeError: unit => jsError = "Error";

[@get] external getStack: jsError => string = "stack";

// Commented to avoid error in webpack
// @module("path") external dirnameFromFilepath: string => string = "dirname"

let dirnameFromFilepath = filepath => {
  filepath
  ->Js.String2.split("/")
  ->Js.Array2.slice(~start=0, ~end_=-1)
  ->Js.Array2.joinWith("/");
};

// Reusable functions that can be simply called from any module instead of
// dealing with import.meta.url etc.

let getFilepathFromError = jsError => {
  let lineWithPath =
    jsError
    ->getStack
    ->Js.String2.split("\n")
    ->Js.Array2.slice(~start=2, ~end_=3)
    ->Belt.Array.get(0);

  switch (lineWithPath) {
  | None => Js.Exn.raiseError("[getFilepathFromError] lineWithPath is None")
  | Some(lineWithPath) =>
    lineWithPath
    ->Js.String2.trim
    ->Js.String2.replace("at file://", "")
    ->Js.String2.replaceByRe(Js.Re.fromString(":[0-9]+:[0-9]+"), "")
  };
};

let getFilepath = () => makeError()->getFilepathFromError;

let getDirname = () => makeError()->getFilepathFromError->dirnameFromFilepath;

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

let maybeAddSlashPrefix = path =>
  if (path->Js.String2.startsWith("http") || path->Js.String2.startsWith("/")) {
    path;
  } else {
    "/" ++ path;
  };

let maybeAddSlashSuffix = path =>
  if (path->Js.String2.endsWith("/")) {
    path;
  } else {
    path ++ "/";
  };
