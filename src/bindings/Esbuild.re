type esbuild;

type plugin;

type buildResult = {
  errors: array(Js.Json.t),
  warnings: array(Js.Json.t),
  metafile: Js.Json.t,
};

[@module "esbuild"] external esbuild: esbuild = "default";

[@send]
external build: (esbuild, Js.t('a)) => Js.Promise.t(buildResult) = "build";

module HtmlPlugin = {
  // https://github.com/craftamap/esbuild-plugin-html/blob/b74debfe7f089a4f073f5a0cf9bbdb2e59370a7c/src/index.ts#L8
  type options = {files: array(htmlFileConfiguration)}
  and htmlFileConfiguration = {
    filename: string,
    entryPoints: array(string),
    htmlTemplate: string,
    scriptLoading: string,
  };

  [@module "@craftamap/esbuild-plugin-html"]
  external make: (. options) => plugin = "htmlPlugin";
};

let makeConfig =
    (
      ~outputDir,
      ~globalEnvValues: array((string, string)),
      ~renderedPages: array(RenderedPage.t),
    ) => {
  "entryPoints": renderedPages->Js.Array2.map(page => page.entryPath),
  "entryNames": Bundler.assetsDirname ++ "/" ++ "js/[dir]/[name]-[hash]",
  "chunkNames": Bundler.assetsDirname ++ "/" ++ "js/_chunks/[name]-[hash]",
  "assetNames": Bundler.assetsDirname ++ "/" ++ "[name]-[hash]",
  "outdir": Bundler.getOutputDir(~outputDir),
  "publicPath": Bundler.assetPrefix,
  "format": "esm",
  "bundle": true,
  "minify": true,
  "metafile": true,
  "splitting": true,
  "logLevel": "error",
  "define": Bundler.getGlobalEnvValuesDict(globalEnvValues),
  "loader": {
    Bundler.assetFileExtensionsWithoutCss
    ->Js.Array2.map(ext => {("." ++ ext, "file")})
    ->Js.Dict.fromArray;
  },
  "plugins": {
    let htmlPluginFiles =
      renderedPages->Js.Array2.map(renderedPage => {
        let pagePath = renderedPage.path->PageBuilderT.PagePath.toString;
        {
          // filename field, which if actually a path will be relative to "outdir".
          HtmlPlugin.filename: pagePath ++ "/index.html",
          // entryPoints must be relative paths to the root of rescript-ssg project
          entryPoints: [|
            Path.relative(
              ~from=Bundler.projectRoot,
              ~to_=renderedPage.entryPath,
            ),
          |],
          htmlTemplate: renderedPage.htmlTemplatePath,
          scriptLoading: "module",
        };
      });

    let htmlPlugin = HtmlPlugin.make(. {files: htmlPluginFiles});

    [|htmlPlugin|];
  },
};

let build =
    (
      ~outputDir,
      ~globalEnvValues: array((string, string)),
      ~renderedPages: array(RenderedPage.t),
    ) => {
  Js.log("[rescript-ssg][Esbuild.build] Bundling...");
  let durationLabel = "[rescript-ssg][Esbuild.build] Success! Duration";
  Js.Console.timeStart(durationLabel);

  let config = makeConfig(~outputDir, ~globalEnvValues, ~renderedPages);

  esbuild
  ->build(config)
  ->Promise.map(_result => {
      // let json = Js.Json.stringifyAny(result.metafile)->Belt.Option.getWithDefault("");
      // Fs.writeFileSync(~path=Path.join2(outputDir, "meta.json"), ~data=json);
      Js.Console.timeEnd(
        durationLabel,
      )
    })
  ->Promise.catch(err => Js.log(err)->Js.Promise.resolve);
};

// Getting a hash of the file contents the same way as it implemented in esbuild.
// 1. Calc xxhash64 from binary data and return digest as binary data.
// Source: https://github.com/evanw/esbuild/blob/main/internal/bundler/bundler.go#L2191-L2195
// 2. Encode binary data to base32.
// Source: https://github.com/evanw/esbuild/blob/main/internal/bundler/bundler.go#L1084-L1086
// Side note: the author of esbuild doesn't want to export hash function to use it from js.
// Maybe it makes sense to raise this question again.
// Source: https://github.com/evanw/esbuild/issues/3113#issuecomment-1542394482
let getFileHash = (buffer: Buffer.t) => {
  HashWasm.createXXHash64AndReturnBinaryDigest(buffer)
  ->Promise.map(buffer => {
      Base32Encode.base32Encode(buffer)->Js.String2.slice(~from=0, ~to_=8)
    });
};
