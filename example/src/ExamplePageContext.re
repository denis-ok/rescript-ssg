type variant =
  | One
  | Two(string);

type polyVariant = [ | `hello | `world];

type t = {
  string,
  int,
  float,
  variant,
  polyVariant,
  bool,
  option: option(string),
};
