// https://nodejs.org/api/util.html#utilinspectobject-options

type options = {
  depth: int,
  colors: bool,
};

[@bs.module "node:util"] external inspect: ('a, options) => string = "inspect";

let inspect = value => inspect(value, {depth: 2, colors: true});
