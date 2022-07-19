type dependencyTreeLibrary;

type tree;

type input = {
  filename: string,
  directory: string,
  filter: string => bool,
};

[@module "dependency-tree"] external make: input => tree = "default";

[@module "dependency-tree"]
external dependencyTreeLibrary: dependencyTreeLibrary = "default";

[@send]
external toList: (dependencyTreeLibrary, input) => array(string) = "toList";

let makeList = input => dependencyTreeLibrary->toList(input);
