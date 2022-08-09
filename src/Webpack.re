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
    [@send] external toJson': (t, string) => Js.Json.t = "toJson";

    let toString = stats =>
      stats->toString'({assets: true, hash: true, colors: true});

    let toJson = stats => stats->toJson'("normal");
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
  path: PageBuilderT.PagePath.t,
  entryPath: string,
  outputDir: string,
  htmlTemplatePath: string,
};

let pages: Js.Dict.t(page) = Js.Dict.empty();

let webpackAssetsDir = "assets";

module DevServerOptions = {
  module Proxy = {
    // https://github.com/DefinitelyTyped/DefinitelyTyped/blob/eefa7b7fce1443e2b6ee5e34d84e142880418208/types/http-proxy/index.d.ts#L25
    type devServerTarget = {
      host: option(string),
      socketPath: option(string),
    };

    type devServerPathRewrite = Js.Dict.t(string);

    type devServerProxyTo = {
      target: devServerTarget,
      pathRewrite: option(devServerPathRewrite),
      secure: bool,
      changeOrigin: bool,
      logLevel: string,
    };

    // https://webpack.js.org/configuration/dev-server/#devserverproxy
    type devServerProxy = Js.Dict.t(devServerProxyTo);

    type target =
      | Host(string)
      | UnixSocket(string);

    type pathRewrite = {
      from: string,
      to_: string,
    };

    type proxyTo = {
      target,
      pathRewrite: option(pathRewrite),
      secure: bool,
      changeOrigin: bool,
    };

    type t = {
      from: string,
      to_: proxyTo,
    };
  };

  type listenTo =
    | Port(int)
    | UnixSocket(string);

  type t = {
    listenTo,
    proxy: option(array(Proxy.t)),
  };
};

let assetRegex = [%re
  "/\\.(jpg|jpeg|png|gif|svg|ico|avif|webp|woff|woff2|json|mp4)$/i"
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
    ->Js.Array2.map(({path, entryPath, _}) =>
        (PageBuilderT.PagePath.toWebpackEntryName(path), entryPath)
      )
    ->Js.Dict.fromArray;

  let htmlWebpackPlugins =
    pages->Js.Array2.map(({path, htmlTemplatePath, _}) => {
      HtmlWebpackPlugin.make({
        "template": htmlTemplatePath,
        "filename":
          Path.join2(PageBuilderT.PagePath.toString(path), "index.html"),
        "chunks": [|PageBuilderT.PagePath.toWebpackEntryName(path)|],
        "inject": true,
        "minify": false,
      })
    });

  let config = {
    "entry": entries,

    "mode": Mode.toString(mode),

    "output": {
      "path": webpackOutputDir,
      "publicPath": "/",
      "filename": "js/[name]_[chunkhash].js",
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
      | Some({listenTo, proxy}) =>
        Some({
          "historyApiFallback": {
            "verbose": true,
            "rewrites": {
              // Here we add support for serving pages with dynamic path parts
              // We use underscore prefix in this library to mark this kind of paths: users/_id
              // Be build rewrite setting below like this:
              // from: "/^\/users\/.*/"
              // to: "/users/_id/index.html"
              let rewrites =
                pages->Belt.Array.keepMap(page =>
                  switch (page.path) {
                  | Root => None
                  | Path(parts) =>
                    let hasDynamicPart =
                      parts
                      ->Js.Array2.find(part =>
                          part->Js.String2.startsWith("_")
                        )
                      ->Belt.Option.isSome;

                    switch (hasDynamicPart) {
                    | false => None
                    | _true =>
                      let pathWithAsterisks =
                        parts
                        ->Js.Array2.map(part =>
                            part->Js.String2.startsWith("_") ? ".*" : part
                          )
                        ->Js.Array2.joinWith("/");

                      let regexString = "^/" ++ pathWithAsterisks;

                      let from = Js.Re.fromString(regexString);

                      let to_ =
                        Path.join3(
                          "/",
                          PageBuilderT.PagePath.toString(page.path),
                          "index.html",
                        );

                      Some({"from": from, "to": to_});
                    };
                  }
                );

              Js.log2("[Webpack dev server] Path rewrites: ", rewrites);
              rewrites;
            },
          },
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
          "proxy": {
            switch (proxy) {
            | None => None
            | Some(proxySettings) =>
              let proxyDict:
                Js.Dict.t(DevServerOptions.Proxy.devServerProxyTo) =
                proxySettings
                ->Js.Array2.map(proxy => {
                    let proxyTo: DevServerOptions.Proxy.devServerProxyTo = {
                      target:
                        switch (proxy.to_.target) {
                        | Host(host) => {host: Some(host), socketPath: None}
                        | UnixSocket(socketPath) => {
                            host: None,
                            socketPath: Some(socketPath),
                          }
                        },
                      pathRewrite: None,
                      secure: proxy.to_.secure,
                      changeOrigin: proxy.to_.changeOrigin,
                      logLevel: "debug",
                    };

                    (proxy.from, proxyTo);
                  })
                ->Js.Dict.fromArray;

              Js.log2("[Webpack dev server] proxyDict: ", proxyDict);

              Some(proxyDict);
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

    let () = {
      let statsJson = Webpack.Stats.toJson(stats);
      Fs.writeFileSync(
        Path.join2(webpackOutputDir, "stats.json"),
        statsJson->Js.Json.stringifyAny->Belt.Option.getWithDefault(""),
      );
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
    Js.Console.error(
      "[Webpack] Can't start dev server, config##devServer is None",
    );
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
