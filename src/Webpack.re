type webpackPlugin;

module HtmlWebpackPlugin = {
  [@module "html-webpack-plugin"] [@new]
  external make: Js.t('a) => webpackPlugin = "default";
};

module CleanWebpackPlugin = {
  [@module "clean-webpack-plugin"] [@new]
  external make: unit => webpackPlugin = "CleanWebpackPlugin";
};

module Webpack = {
  type compiler;

  [@module "webpack"]
  external makeCompiler: Js.t({..}) => compiler = "default";

  [@send] external run: (compiler, ('err, 'stats) => unit) => unit = "run";

  [@send] external close: (compiler, 'closeError => unit) => unit = "close";
};

module WebpackDevServer = {
  type t;

  [@new] [@module "webpack-dev-server"]
  external make: (Js.t({..}), Webpack.compiler) => t = "default";

  [@send]
  external startWithCallback: (t, unit => unit) => unit = "startCallback";

  [@send]
  external stopWithCallback: (t, unit => unit) => unit = "stopCallback";
};

type page = {
  title: string,
  slug: string,
  entryPath: string,
  outputDir: string,
  htmlTemplatePath: string,
};

let pages: Js.Dict.t(page) = Js.Dict.empty();

let isProduction = false;

let makeConfig = (~webpackOutputDir) => {
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

let makeCompiler = (~webpackOutputDir) => {
  let config = makeConfig(~webpackOutputDir);
  let compiler = Webpack.makeCompiler(config);
  (compiler, config);
};

let build = (~webpackOutputDir) => {
  let (compiler, _config) = makeCompiler(~webpackOutputDir);

  compiler->Webpack.run((err, _stats) => {
    switch (Js.Nullable.toOption(err)) {
    | None => Js.log("[Webpack] Build success")
    | Some(_error) => Js.log("[Webpack] Build error")
    };

    compiler->Webpack.close(closeError => {
      switch (Js.Nullable.toOption(closeError)) {
      | None => ()
      | Some(_error) => Js.log("[Webpack] Compiler close error")
      }
    });
  });
};

let startDevServer = (~webpackOutputDir) => {
  let (compiler, config) = makeCompiler(~webpackOutputDir);
  let devServerOptions = config##devServer;
  let devServer = WebpackDevServer.make(devServerOptions, compiler);

  devServer->WebpackDevServer.startWithCallback(() => {
    Js.log("[Webpack] WebpackDevServer started")
  });
  //
  // Js.Global.setTimeout(
  //   () =>
  //     devServer->WebpackDevServer.stopWithCallback(() => {
  //       Js.log("Dev server stopped!")
  //     }),
  //   5000,
  // )
  // ->ignore;
};
