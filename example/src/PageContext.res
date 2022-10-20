type variant =
  | One
  | Two(string)

type polyVariant = [#hello | #world]

type t = {
  string: string,
  int: int,
  float: float,
  variant: variant,
  polyVariant: polyVariant,
  bool: bool,
  option: option<string>,
}
