type mode =
  | Build
  | Watch;

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
  assetFileExtensions->Js.Array.filter(~f=ext => ext !== "css", _);

let assetRegex = {
  let regex: string = assetFileExtensions->Js.Array.join(~sep="|", _);
  let regex = {|\.|} ++ "(" ++ regex ++ ")" ++ "$";
  Js.Re.fromStringWithFlags(regex, ~flags="i");
};

let getGlobalEnvValuesDict = (globalEnvValues: array((string, string))) => {
  let dict = Js.Dict.empty();

  globalEnvValues->Js.Array.forEach(~f=((key, value)) => {
    let value = {j|"$(value)"|j};
    dict->Js.Dict.set(key, value);
  }, _);

  dict;
};

let getOutputDir = (~outputDir) => Path.join2(outputDir, "public");

let assetPrefix =
  EnvParams.assetPrefix->Utils.maybeAddSlashPrefix->Utils.maybeAddSlashSuffix;
