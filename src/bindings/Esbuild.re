type esbuild;
type config;
type esbuildPluginSvgr;

[@module "esbuild"] external esbuild: esbuild = "default";

[@module "esbuild-plugin-svgr"]
external esbuildPluginSvgr: (. Js.t('a)) => unit = "default";

[@send] external build: (esbuild, Js.t('a)) => Js.Promise.t('a) = "build";

let makeConfig = (~outputDir, ~renderedPages: array(RenderedPage.t)) => {
  "entryPoints": renderedPages->Js.Array2.map(page => page.entryPath),
  "nodePaths": [|"../node_modules"|],
  "bundle": true,
  "minify": true,
  "metafile": true,
  "plugins": [|
    esbuildPluginSvgr(. {
      "dimensions": false,
      "plugins": ["@svgr/plugin-jsx"],
      "svgo": false,
      "titleProp": true,
    }),
  |],
  "loader": {
    ".svg": "file",
    ".jpeg": "file",
    ".jpg": "file",
    ".png": "file",
  },
  "splitting": true,
  "format": "esm",
  "logLevel": "error",
  "outdir": Path.join2(outputDir, "esbuild"),
};

let build = (~outputDir, ~renderedPages: array(RenderedPage.t)) => {
  let config = makeConfig(~outputDir, ~renderedPages);

  esbuild
  ->build(config)
  ->Promise.map(result => {
      let json =
        Js.Json.stringifyAny(result)->Belt.Option.getWithDefault("");
      Fs.writeFileSync(~path=Path.join2(outputDir, "meta.json"), ~data=json);
    })
  ->Promise.catch(err => Js.log(err)->Js.Promise.resolve);
};

let getFileHash = (buffer: Buffer.t) => {
  HashWasm.createXXHash64AndReturnBinaryDigest(buffer)
  ->Promise.map(buffer => {
      Base32Encode.base32Encode(buffer)->Js.String2.slice(~from=0, ~to_=8)
    });
};
