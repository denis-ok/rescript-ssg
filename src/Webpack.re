type webpackPlugin;

module HtmlWebpackPlugin = {
  [@module "html-webpack-plugin"] [@new]
  external make: Js.t('a) => webpackPlugin = "default";
};

module CleanWebpackPlugin = {
  [@module "clean-webpack-plugin"] [@new]
  external make: unit => webpackPlugin = "CleanWebpackPlugin";
};

type webpackCompiler;

// [@module "webpack"]
// external makeCompilerWithCallback:
//   (Js.t({..}), ('err, 'stats) => unit) => webpackCompiler =
//   "default";

[@module "webpack"]
external makeCompiler: Js.t({..}) => webpackCompiler = "default";

type devServer;

[@new] [@module "webpack-dev-server"]
external makeWebpackDevServer: (Js.t({..}), webpackCompiler) => devServer =
  "default";

[@send]
external startCallback: (devServer, unit => unit) => unit = "startCallback";

[@send]
external stopCallback: (devServer, unit => unit) => unit = "stopCallback";

type page = {
  title: string,
  slug: string,
  entryPath: string,
  outputDir: string,
  htmlTemplatePath: string,
};

let pages: Js.Dict.t(page) = Js.Dict.empty();

let exampleBuildDir = Path.join2(SrcPath.srcPath, "../example/build");

let webpackOutputDir = Path.join2(exampleBuildDir, "bundle");

let isProduction = false;

let makeConfig = () => {
  let pages = pages->Js.Dict.values;

  let entries =
    pages
    ->Js.Array2.map(({slug, entryPath}) => (slug, entryPath))
    ->Js.Dict.fromArray;

  let htmlWebpackPlugins =
    pages->Js.Array2.map(({title, slug, htmlTemplatePath}) =>
      HtmlWebpackPlugin.make({
        "title": title,
        "lang": "en",
        "template": htmlTemplatePath,
        "filename":
          slug == "index" ? "./index.html" : {j|$(slug)/index.html|j},
        "chunks": [|slug|],
        "inject": true,
        "minify": false,
      })
    );

  let config = {
    "entry": entries,

    "mode": isProduction ? "production" : "development",

    "output": {
      "path": webpackOutputDir,
      "filename": "js/[name].[chunkhash].js",
    },

    "module": {
      "rules": [||],
    },

    "plugins":
      Js.Array2.concat([|CleanWebpackPlugin.make()|], htmlWebpackPlugins),

    "devServer": {
      "historyApiFallback": true,
      "hot": false,
      // static: {
      //   directory: path.join(__dirname, "public"),
      // },
      // compress: true,
      "port": 9007,
    },
  };

  config;
};

let startDevServer = () => {
  let config = makeConfig();

  let compiler = makeCompiler(config);

  let devServerOptions = config##devServer;

  let devServer = makeWebpackDevServer(devServerOptions, compiler);

  devServer->startCallback(() => {Js.log("Dev server started")});

  // Js.Global.setTimeout(
  //   () => devServer->stopCallback(() => {Js.log("Dev server stopped!")}),
  //   5000,
  // )
  // ->ignore;

  ();
};
