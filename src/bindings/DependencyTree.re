type dependencyTreeLibrary;

type tree;

type input = {
  filename: string,
  directory: string,
  filter: string => bool,
};

[@bs.module "dependency-tree"] external make: input => tree = "default";

[@bs.module "dependency-tree"]
external dependencyTreeLibrary: dependencyTreeLibrary = "default";

[@bs.send]
external toList: (dependencyTreeLibrary, input) => array(string) = "toList";

let makeList = input => dependencyTreeLibrary->toList(input);
