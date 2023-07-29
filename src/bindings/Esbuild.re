type esbuild;
type config;
type esbuildPluginSvgr;

[@module "esbuild"] external esbuild: esbuild = "default";

[@module "esbuild-plugin-svgr"]
external esbuildPluginSvgr: (. Js.t('a)) => unit = "default";

[@send] external build: (esbuild, Js.t('a)) => Js.Promise.t('a) = "build";

type webpackPages = array(Webpack.page);

let makeConfig = (~outputDir, ~webpackPages: webpackPages) => {
  "entryPoints": webpackPages->Js.Array2.map(page => page.entryPath),
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

let build = (~outputDir, ~webpackPages: webpackPages) => {
  let config = makeConfig(~outputDir, ~webpackPages);
  esbuild
  ->build(config)
  ->Promise.map(result => {
      let json =
        Js.Json.stringifyAny(result)->Belt.Option.getWithDefault("");
      Fs.writeFileSync(~path=Path.join2(outputDir, "meta.json"), ~data=json);
    })
  ->Promise.catch(err => Js.log(err)->Js.Promise.resolve);
};
