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

let assetsDirname = "assets";

let assetFileExtensions = [|
  "css",
  "jpg",
  "jpeg",
  "png",
  "gif",
  "svg",
  "ico",
  "avif",
  "webp",
  "woff",
  "woff2",
  "json",
  "mp4",
|];

let assetFileExtensionsWithoutCss =
  assetFileExtensions->Js.Array2.filter(ext => ext !== "css");

let assetRegex = {
  let regex: string = assetFileExtensions->Js.Array2.joinWith("|");
  let regex = {|\.|} ++ "(" ++ regex ++ ")" ++ "$";
  Js.Re.fromStringWithFlags(regex, ~flags="i");
};
