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
  module Stats = {
    type t;

    type toStringOptions = {
      assets: bool,
      hash: bool,
      colors: bool,
    };

    [@send] external hasErrors: t => bool = "hasErrors";
    [@send] external hasWarnings: t => bool = "hasWarnings";
    [@send] external toString': (t, toStringOptions) => string = "toString";

    let toString = stats =>
      stats->toString'({assets: true, hash: true, colors: true});
  };

  type compiler;

  [@module "webpack"]
  external makeCompiler: Js.t({..}) => compiler = "default";

  [@send] external run: (compiler, ('err, Stats.t) => unit) => unit = "run";

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

module Mode = {
  type t =
    | Development
    | Production;

  let toString = (mode: t) =>
    switch (mode) {
    | Development => "development"
    | Production => "production"
    };
};

module Hash = {
  type crypto;

  type hash;

  [@module "crypto"] external crypto: crypto = "default";

  [@send "createHash"]
  external createHash': (crypto, string) => hash = "createHash";

  [@send "update"] external update': (hash, string) => hash = "update";

  [@send "digest"] external digest': (hash, string) => string = "digest";

  let digestLength = 20;

  let makeNew = () => crypto->createHash'("md4");

  let dataToHash = data =>
    crypto
    ->createHash'("md4")
    ->update'(data)
    ->digest'("hex")
    ->Js.String2.slice(~from=0, ~to_=digestLength);
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

let webpackAssetsDir = "assets";

module DevServerOptions = {
  type listenTo =
    | Port(int)
    | UnixSocket(string);

  type t = {listenTo};
};

let assetRegex = [%re
  "/\\.(jpg|jpeg|png|gif|svg|ico|avif|webp|woff|woff2|json)$/i"
];

let makeConfig =
    (
      ~devServerOptions: option(DevServerOptions.t),
      ~mode: Mode.t,
      ~webpackOutputDir: string,
    ) => {
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

    "mode": Mode.toString(mode),

    "output": {
      "path": webpackOutputDir,
      "publicPath": "",
      "filename": "js/[name].[chunkhash].js",
      // Hash suffix disabled.
      // TODO Figure out how to use custom hash func to reuse in node-loader.
      "assetModuleFilename": webpackAssetsDir ++ "/" ++ "[name].[hash][ext]",
      "hashFunction": Hash.makeNew,
      "hashDigestLength": Hash.digestLength,
    },

    "module": {
      "rules": [|
        {
          //
          "test": assetRegex,
          "type": "asset/resource",
        },
      |],
    },

    "plugins":
      Js.Array2.concat([|CleanWebpackPlugin.make()|], htmlWebpackPlugins),

    "devServer": {
      switch (devServerOptions) {
      | None => None
      | Some({listenTo}) =>
        Some({
          "historyApiFallback": true,
          "hot": false,
          // static: {
          //   directory: path.join(__dirname, "public"),
          // },
          // compress: true,
          "port": {
            switch (listenTo) {
            | Port(port) => Some(port)
            | UnixSocket(_) => None
            };
          },
          // TODO Should we check/remove socket file before starting or on terminating dev server?
          "ipc": {
            switch (listenTo) {
            | UnixSocket(path) => Some(path)
            | Port(_) => None
            };
          },
        })
      };
    },
  };

  config;
};

let makeCompiler = (~devServerOptions, ~mode, ~webpackOutputDir) => {
  let config = makeConfig(~devServerOptions, ~mode, ~webpackOutputDir);
  // TODO handle errors when we make compiler
  let compiler = Webpack.makeCompiler(config);
  (compiler, config);
};

let build = (~mode, ~webpackOutputDir, ~verbose) => {
  let (compiler, _config) =
    makeCompiler(~devServerOptions=None, ~mode, ~webpackOutputDir);

  compiler->Webpack.run((err, stats) => {
    switch (Js.Nullable.toOption(err)) {
    | None => Js.log("[Webpack] Build success")
    | Some(_error) => Js.log("[Webpack] Build error")
    };

    switch (Webpack.Stats.hasErrors(stats)) {
    | true => Js.log("[Webpack] Stats.hasErrors")
    | _ => ()
    };

    switch (Webpack.Stats.hasWarnings(stats)) {
    | true => Js.log("[Webpack] Stats.hasWarnings")
    | _ => ()
    };

    if (verbose) {
      Js.log(Webpack.Stats.toString(stats));
    };

    compiler->Webpack.close(closeError => {
      switch (Js.Nullable.toOption(closeError)) {
      | None => ()
      | Some(_error) => Js.log("[Webpack] Compiler close error")
      }
    });
  });
};

let startDevServer =
    (~devServerOptions: DevServerOptions.t, ~mode, ~webpackOutputDir) => {
  let (compiler, config) =
    makeCompiler(
      ~devServerOptions=Some(devServerOptions),
      ~mode,
      ~webpackOutputDir,
    );

  let devServerOptions = config##devServer;
  switch (devServerOptions) {
  | None =>
    Js.Console.error("Can't start dev server, config##devServer is None");
    Process.exit(1);
  | Some(devServerOptions) =>
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
};
