type webpackPlugin;

module HtmlWebpackPlugin = {
  [@module "html-webpack-plugin"] [@new]
  external make: Js.t('a) => webpackPlugin = "default";
};

[@new] [@module "webpack"] [@scope "default"]
external definePlugin: Js.Dict.t(string) => webpackPlugin = "DefinePlugin";

[@new] [@module "webpack/lib/debug/ProfilingPlugin.js"]
external makeProfilingPlugin: unit => webpackPlugin = "default";

[@new] [@module "esbuild-loader"]
external makeESBuildPlugin: Js.t('a) => webpackPlugin = "EsbuildPlugin";

[@val] external processEnvDict: Js.Dict.t(string) = "process.env";

let getPluginWithGlobalValues = (globalValues: array((string, string))) => {
  let keyBase = "process.env";

  let makeKey = varName => keyBase ++ "." ++ varName;

  let envItems = processEnvDict->Js.Dict.entries;

  let dict = Js.Dict.empty();

  dict->Js.Dict.set(keyBase, "({})");

  envItems->Js.Array2.forEach(((key, value)) => {
    let key = makeKey(key);
    let value = {j|"$(value)"|j};
    dict->Js.Dict.set(key, value);
  });

  globalValues->Js.Array2.forEach(((key, value)) => {
    let value = {j|"$(value)"|j};
    dict->Js.Dict.set(key, value);
  });

  definePlugin(dict);
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

module Minimizer = {
  type t =
    | Terser
    | Esbuild;
};

type page = {
  path: PageBuilderT.PagePath.t,
  entryPath: string,
  outputDir: string,
  htmlTemplatePath: string,
};

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

let getWebpackOutputDir = outputDir => Path.join2(outputDir, "public");

let makeConfig =
    (
      ~devServerOptions: option(DevServerOptions.t),
      ~mode: Mode.t,
      ~minimizer: Minimizer.t,
      ~logger: Log.logger,
      ~outputDir: string,
      ~globalValues: array((string, string)),
      ~webpackPages: array(page),
    ) => {
  let entries =
    webpackPages
    ->Js.Array2.map(({path, entryPath, _}) =>
        (PageBuilderT.PagePath.toWebpackEntryName(path), entryPath)
      )
    ->Js.Dict.fromArray;

  let assetPrefix =
    CliArgs.assetPrefix->Utils.maybeAddSlashPrefix->Utils.maybeAddSlashSuffix;

  let shouldMinimize = mode == Production;

  let config = {
    "entry": entries,

    "mode": Mode.toString(mode),

    "output": {
      "path": getWebpackOutputDir(outputDir),
      "publicPath": assetPrefix,
      "filename":
        NodeLoader.webpackAssetsDir ++ "/" ++ "js/[name]_[chunkhash].js",
      "assetModuleFilename":
        NodeLoader.webpackAssetsDir ++ "/" ++ "[name].[hash][ext]",
      "hashFunction": Crypto.Hash.createMd5,
      "hashDigestLength": Crypto.Hash.digestLength,
      // Clean the output directory before emit.
      "clean": true,
    },

    "module": {
      "rules": [|
        {
          //
          "test": NodeLoader.assetRegex,
          "type": "asset/resource",
        },
      |],
    },

    "plugins": {
      let htmlWebpackPlugins =
        webpackPages->Js.Array2.map(({path, htmlTemplatePath, _}) => {
          HtmlWebpackPlugin.make({
            "template": htmlTemplatePath,
            "filename":
              Path.join2(PageBuilderT.PagePath.toString(path), "index.html"),
            "chunks": [|PageBuilderT.PagePath.toWebpackEntryName(path)|],
            "inject": true,
            "minify": {
              "collapseWhitespace": shouldMinimize,
              "keepClosingSlash": shouldMinimize,
              "removeComments": shouldMinimize,
              "removeRedundantAttributes": shouldMinimize,
              "removeScriptTypeAttributes": shouldMinimize,
              "removeStyleLinkTypeAttributes": shouldMinimize,
              "useShortDoctype": shouldMinimize,
              "minifyCSS": shouldMinimize,
            },
          })
        });

      let browserEnvPlugin = getPluginWithGlobalValues(globalValues);

      Js.Array2.concat([|browserEnvPlugin|], htmlWebpackPlugins);
    },
    // Explicitly disable source maps in dev mode
    "devtool": false,
    "optimization": {
      "runtimeChunk": {
        "name": "webpack-runtime",
      },
      "minimize": shouldMinimize,
      "minimizer": {
        switch (shouldMinimize, minimizer) {
        | (true, Esbuild) =>
          Some([|makeESBuildPlugin({"target": "es2015"})|])
        | (false, _)
        | (_, Terser) => None
        };
      },
      "splitChunks": {
        "chunks": "all",
        "minSize": 20000,
        "cacheGroups": {
          "framework": {
            "priority": 40,
            "name": "framework",
            "test": {
              let frameworkPackages =
                [|"react", "react-dom", "scheduler", "prop-types"|]
                ->Js.Array2.joinWith("|");
              let regexStr = {j|(?<!node_modules.*)[\\\\/]node_modules[\\\\/]($(frameworkPackages))[\\\\/]|j};
              let regex = Js.Re.fromString(regexStr);
              regex;
            },
            "enforce": true,
          },
          "react-helmet": {
            "priority": 30,
            "name": "react-helmet",
            "test": {
              let packages = [|"react-helmet"|]->Js.Array2.joinWith("|");
              let regexStr = {j|[\\\\/]node_modules[\\\\/]($(packages))[\\\\/]|j};
              let regex = Js.Re.fromString(regexStr);
              regex;
            },
            "enforce": true,
          },
        },
      },
    },
    "watchOptions": {
      "aggregateTimeout": 1000,
    },
    "devServer": {
      switch (devServerOptions) {
      | None => None
      | Some({listenTo, proxy}) =>
        Some({
          "devMiddleware": {
            "stats": {
              switch (logger.logLevel) {
              | Info => "errors-warnings"
              | Debug => "normal"
              };
            },
          },
          "historyApiFallback": {
            "verbose": true,
            "rewrites": {
              // Here we add support for serving pages with dynamic path parts
              // We use underscore prefix in this library to mark this kind of paths: users/_id
              // Be build rewrite setting below like this:
              // from: "/^\/users\/.*/"
              // to: "/users/_id/index.html"
              let rewrites =
                webpackPages->Belt.Array.keepMap(page =>
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

              logger.info(() =>
                Js.log2("[Webpack dev server] Path rewrites: ", rewrites)
              );
              rewrites;
            },
          },
          "hot": false,
          // static: {
          //   directory: path.join(__dirname, "public"),
          // },
          "compress": true,
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

              logger.debug(() =>
                Js.log2("[Webpack dev server] proxyDict: ", proxyDict)
              );

              Some(proxyDict);
            };
          },
        })
      };
    },
  };

  config;
};

let makeCompiler =
    (
      ~devServerOptions: option(DevServerOptions.t),
      ~logger: Log.logger,
      ~mode: Mode.t,
      ~minimizer: Minimizer.t,
      ~globalValues: array((string, string)),
      ~outputDir,
      ~webpackPages: array(page),
    ) => {
  let config =
    makeConfig(
      ~devServerOptions,
      ~mode,
      ~logger,
      ~minimizer,
      ~outputDir,
      ~globalValues,
      ~webpackPages,
    );
  // TODO handle errors when we make compiler
  let compiler = Webpack.makeCompiler(config);
  (compiler, config);
};

let build =
    (
      ~mode: Mode.t,
      ~minimizer: Minimizer.t,
      ~writeWebpackStatsJson: bool,
      ~logger: Log.logger,
      ~outputDir,
      ~globalValues: array((string, string)),
      ~webpackPages: array(page),
    ) => {
  let durationLabel = "[Webpack.build] duration";
  Js.Console.timeStart(durationLabel);

  logger.info(() => Js.log("[Webpack.build] Building webpack bundle..."));

  let (compiler, _config) =
    makeCompiler(
      ~devServerOptions=None,
      ~mode,
      ~logger,
      ~outputDir,
      ~minimizer,
      ~globalValues,
      ~webpackPages,
    );

  compiler->Webpack.run((err, stats) => {
    switch (Js.Nullable.toOption(err)) {
    | None => logger.info(() => {Js.log("[Webpack.build] Success!")})
    | Some(_error) => logger.info(() => Js.log("[Webpack.build] Error!"))
    };

    switch (Webpack.Stats.hasErrors(stats)) {
    | true => logger.info(() => Js.log("[Webpack.build] Stats.hasErrors"))
    | _ => ()
    };

    switch (Webpack.Stats.hasWarnings(stats)) {
    | true => logger.info(() => Js.log("[Webpack.build] Stats.hasWarnings"))
    | _ => ()
    };

    logger.info(() => Js.log(Webpack.Stats.toString(stats)));

    let () = {
      let webpackOutputDir = getWebpackOutputDir(outputDir);

      if (writeWebpackStatsJson) {
        let statsJson = Webpack.Stats.toJson(stats);
        Fs.writeFileSync(
          Path.join2(webpackOutputDir, "stats.json"),
          statsJson->Js.Json.stringifyAny->Belt.Option.getWithDefault(""),
        );
      };
    };

    compiler->Webpack.close(closeError => {
      switch (Js.Nullable.toOption(closeError)) {
      | None =>
        Js.Console.timeEnd(durationLabel);
        ();
      | Some(_error) =>
        logger.info(() => Js.log("[Webpack.build] Compiler close error"))
      }
    });
  });
};

let startDevServer =
    (
      ~devServerOptions: DevServerOptions.t,
      ~mode: Mode.t,
      ~minimizer: Minimizer.t,
      ~logger: Log.logger,
      ~outputDir,
      ~globalValues: array((string, string)),
      ~webpackPages: array(page),
    ) => {
  logger.info(() => Js.log("[Webpack] Starting dev server..."));
  let startupDurationLabel = "[Webpack] WebpackDevServer startup duration";
  Js.Console.timeStart(startupDurationLabel);

  let (compiler, config) =
    makeCompiler(
      ~devServerOptions=Some(devServerOptions),
      ~mode,
      ~logger,
      ~outputDir,
      ~minimizer,
      ~globalValues,
      ~webpackPages,
    );

  let devServerOptions = config##devServer;

  switch (devServerOptions) {
  | None =>
    logger.info(() =>
      Js.Console.error(
        "[Webpack] Can't start dev server, config##devServer is None",
      )
    );
    Process.exit(1);
  | Some(devServerOptions) =>
    let devServer = WebpackDevServer.make(devServerOptions, compiler);
    devServer->WebpackDevServer.startWithCallback(() => {
      logger.info(() => {
        Js.log("[Webpack] WebpackDevServer started");
        Js.Console.timeEnd(startupDurationLabel);
      })
    });
  };
};
