type dependencyTreeLibrary;

type tree;

type input = {
  filename: string,
  directory: string,
  filter: string => bool,
};

[@mel.module "dependency-tree"] external make: input => tree = "default";

[@mel.module "dependency-tree"]
external dependencyTreeLibrary: dependencyTreeLibrary = "default";

[@mel.send]
external toList: (dependencyTreeLibrary, input) => array(string) = "toList";

let makeList = input => dependencyTreeLibrary->toList(input);
