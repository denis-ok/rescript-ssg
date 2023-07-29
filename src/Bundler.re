type t =
  | Webpack
  | Esbuild;

let fromString = (bundler: string) =>
  switch (bundler) {
  | "webpack" => Webpack
  | "esbuild" => Esbuild
  | _ => Webpack
  };

let bundler =
  Process.env
  ->Js.Dict.get("RESCRIPT_SSG_BUNDLER")
  ->Belt.Option.getWithDefault("")
  ->Js.String2.toLowerCase
  ->fromString;
