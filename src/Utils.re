type jsError;

[@mel.new] external makeError: unit => jsError = "Error";

[@mel.get] external getStack: jsError => string = "stack";

external window: _ = "window";

// Commented to avoid error in webpack
// @module("path") external dirnameFromFilepath: string => string = "dirname"

let dirnameFromFilepath = filepath => {
  filepath
  ->Js.String.split(~sep="/", _)
  ->Js.Array.slice(~start=0, ~end_=-1, _)
  ->Js.Array.join(~sep="/", _);
};

// Reusable functions that can be simply called from any module instead of
// dealing with import.meta.url etc.

let getFilepathFromError = jsError => {
  let lineWithPath =
    jsError
    ->getStack
    ->Js.String.split(~sep="\n", _)
    ->Js.Array.slice(~start=2, ~end_=3, _)
    ->Belt.Array.get(0);

  switch (lineWithPath) {
  | None => Js.Exn.raiseError("[getFilepathFromError] lineWithPath is None")
  | Some(lineWithPath) =>
    lineWithPath
    ->Js.String.trim
    ->Js.String.replace(~search="at file://", ~replacement="", _)
    ->Js.String.replaceByRe(~regexp=Js.Re.fromString(":[0-9]+:[0-9]+"), ~replacement="", _)
  };
};

let getFilepath = () =>
  switch (Js.typeof(window) == "undefined") {
  // Get filepath only in node
  | false => ""
  | true => makeError()->getFilepathFromError
  };

let getDirname = () => makeError()->getFilepathFromError->dirnameFromFilepath;

let getModuleNameFromModulePath = modulePath => {
  let segments = modulePath->Js.String.split(~sep="/", _);

  let filename =
    segments->Js.Array.copy->Js.Array.reverseInPlace->Belt.Array.get(0);

  switch (filename) {
  | None
  | Some("") =>
    Js.Console.error(
      "[Utils.getModuleNameFromModulePath] Filename is empty or None",
    );
    Process.exit(1);
  | Some(filename) => filename->Js.String.replace(~search=".bs.js", ~replacement="")
  };
};

let maybeAddSlashPrefix = path =>
  if (path->Js.String.startsWith(~prefix="http", _) || path->Js.String.startsWith(~prefix="/", _)) {
    path;
  } else {
    "/" ++ path;
  };

let maybeAddSlashSuffix = path =>
  if (path->Js.String.endsWith(~suffix="/", _)) {
    path;
  } else {
    path ++ "/";
  };
