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
