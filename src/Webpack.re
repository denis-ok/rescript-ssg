type webpackPlugin;

module NodeLoader = NodeLoader; /* Workaround bug in dune and melange: https://github.com/ocaml/dune/pull/6625 */
module Crypto = Crypto; /* Workaround bug in dune and melange: https://github.com/ocaml/dune/pull/6625 */

module HtmlWebpackPlugin = {
  [@mel.module "html-webpack-plugin"] [@mel.new]
  external make: Js.t('a) => webpackPlugin = "default";
};

module MiniCssExtractPlugin = {
  [@mel.module "mini-css-extract-plugin"] [@mel.new]
  external make: Js.t('a) => webpackPlugin = "default";

  [@mel.module "mini-css-extract-plugin"] [@mel.scope "default"]
  external loader: string = "loader";
};

module TerserPlugin = {
  type minifier;
  [@mel.module "terser-webpack-plugin"] [@mel.new]
  external make: Js.t('a) => webpackPlugin = "default";
  [@mel.module "terser-webpack-plugin"] [@mel.scope "default"]
  external swcMinify: minifier = "swcMinify";
  [@mel.module "terser-webpack-plugin"] [@mel.scope "default"]
  external esbuildMinify: minifier = "esbuildMinify";
};

module WebpackBundleAnalyzerPlugin = {
  // https://github.com/webpack-contrib/webpack-bundle-analyzer#options-for-plugin

  type analyzerMode = [ | `server | `static | `json | `disabled];

  type options = {
    analyzerMode,
    reportFilename: option(string),
    openAnalyzer: bool,
    analyzerPort: option(int),
  };

  [@mel.module "webpack-bundle-analyzer"] [@mel.new]
  external make': options => webpackPlugin = "BundleAnalyzerPlugin";

  module Mode = {
    // This module contains an interface that is exposed by rescript-ssg
    type staticModeOptions = {reportHtmlFilepath: string};

    type serverModeOptions = {port: int};

    type t =
      | Static(staticModeOptions)
      | Server(serverModeOptions);

    let makeOptions = (mode: t) => {
      switch (mode) {
      | Static({reportHtmlFilepath}) => {
          analyzerMode: `static,
          reportFilename: Some(reportHtmlFilepath),
          openAnalyzer: false,
          analyzerPort: None,
        }
      | Server({port}) => {
          analyzerMode: `server,
          reportFilename: None,
          openAnalyzer: false,
          analyzerPort: Some(port),
        }
      };
    };
  };

  let make = (mode: Mode.t) => mode->Mode.makeOptions->make';
};

[@mel.new] [@mel.module "webpack"] [@mel.scope "default"]
external definePlugin: Js.Dict.t(string) => webpackPlugin = "DefinePlugin";

[@mel.new] [@mel.module "webpack/lib/debug/ProfilingPlugin.js"]
external makeProfilingPlugin: unit => webpackPlugin = "default";

[@mel.new] [@mel.module "esbuild-loader"]
external makeESBuildPlugin: Js.t('a) => webpackPlugin = "EsbuildPlugin";

let getPluginWithGlobalValues =
    (globalEnvValuesDict: array((string, string))) => {
  Bundler.getGlobalEnvValuesDict(globalEnvValuesDict)->definePlugin;
};

module Webpack = {
  module Stats = {
    type t;

    type toStringOptions = {
      assets: bool,
      hash: bool,
      colors: bool,
    };

    [@mel.send] external hasErrors: t => bool = "hasErrors";
    [@mel.send] external hasWarnings: t => bool = "hasWarnings";
    [@mel.send] external toString': (t, toStringOptions) => string = "toString";
    [@mel.send] external toJson': (t, string) => Js.Json.t = "toJson";

    let toString = stats =>
      stats->toString'({assets: true, hash: true, colors: true});

    let toJson = stats => stats->toJson'("normal");
  };

  type compiler;

  [@mel.module "webpack"]
  external makeCompiler: Js.t({..}) => compiler = "default";

  [@mel.send]
  external run: (compiler, ('err, Js.Nullable.t(Stats.t)) => unit) => unit =
    "run";

  [@mel.send] external close: (compiler, 'closeError => unit) => unit = "close";
};

module WebpackDevServer = {
  type t;

  [@mel.new] [@mel.module "webpack-dev-server"]
  external make: (Js.t({..}), Webpack.compiler) => t = "default";

  [@mel.send]
  external startWithCallback: (t, unit => unit) => unit = "startCallback";

  [@mel.send] external stop: (t, unit) => Js.Promise.t(unit) = "stop";
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
    | EsbuildPlugin
    | TerserPluginWithEsbuild;
};

module DevServerOptions = {
  module Proxy = {
    module DevServerTarget = {
      // https://github.com/DefinitelyTyped/DefinitelyTyped/blob/eefa7b7fce1443e2b6ee5e34d84e142880418208/types/http-proxy/index.d.ts#L25
      type params = {
        host: option(string),
        socketPath: option(string),
      };

      type t =
        | String(string)
        | Params(params);

      [@unboxed]
      type unboxed =
        | Any('a): unboxed;

      let makeUnboxed = (devServerTarget: t) =>
        switch (devServerTarget) {
        | String(s) => Any(s)
        | Params(devServerTarget) => Any(devServerTarget)
        };
    };

    type devServerPathRewrite = Js.Dict.t(string);

    type devServerProxyTo = {
      target: DevServerTarget.unboxed,
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

let makeConfig =
    (
      ~webpackBundleAnalyzerMode: option(WebpackBundleAnalyzerPlugin.Mode.t),
      ~webpackDevServerOptions: option(DevServerOptions.t),
      ~webpackMode: Mode.t,
      ~webpackMinimizer: Minimizer.t,
      ~logger: Log.logger,
      ~outputDir: string,
      ~globalEnvValues: array((string, string)),
      ~renderedPages: array(RenderedPage.t),
    ) => {
  let entries =
    renderedPages
    ->Js.Array.map(~f=({path, entryPath, _}: RenderedPage.t) =>
        (PagePath.toWebpackEntryName(path), entryPath)
      , _)
    ->Js.Dict.fromArray;

  let shouldMinimize = webpackMode == Production;

  let config = {
    "entry": entries,

    "mode": Mode.toString(webpackMode),

    "output": {
      "path": Bundler.getOutputDir(~outputDir),
      "publicPath": Bundler.assetPrefix,
      "filename": Bundler.assetsDirname ++ "/" ++ "js/[name]_[chunkhash].js",
      "assetModuleFilename":
        Bundler.assetsDirname ++ "/" ++ "[name].[hash][ext]",
      "hashFunction": Crypto.Hash.createMd5,
      "hashDigestLength": Crypto.Hash.digestLength,
      // Clean the output directory before emit.
      "clean": true,
    },

    "module": {
      "rules": [|
        {
          //
          "test": [%re {|/\.css$/|}],
          "use": [|MiniCssExtractPlugin.loader, "css-loader"|],
        },
        {"test": Bundler.assetRegex, "type": "asset/resource"}->Obj.magic,
      |],
    },

    "plugins": {
      let htmlWebpackPlugins =
        renderedPages->Js.Array.map(~f=({path, htmlTemplatePath, _}: RenderedPage.t) => {
          HtmlWebpackPlugin.make({
            "template": htmlTemplatePath,
            "filename": Path.join2(PagePath.toString(path), "index.html"),
            "chunks": [|PagePath.toWebpackEntryName(path)|],
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
        }, _);

      let globalValuesPlugin = getPluginWithGlobalValues(globalEnvValues);

      let miniCssExtractPlugin =
        MiniCssExtractPlugin.make({
          "filename": Bundler.assetsDirname ++ "/" ++ "[name]_[chunkhash].css",
        });

      let webpackBundleAnalyzerPlugin =
        switch (webpackBundleAnalyzerMode) {
        | None => [||]
        | Some(mode) => [|WebpackBundleAnalyzerPlugin.make(mode)|]
        };

      Js.Array.concat(
        ~other=[|miniCssExtractPlugin, globalValuesPlugin|],
        htmlWebpackPlugins,
      )
      ->Js.Array.concat(~other=webpackBundleAnalyzerPlugin, _);
    },
    // Explicitly disable source maps in dev mode
    "devtool": false,
    "optimization": {
      "runtimeChunk": {
        "name": "webpack-runtime",
      },
      "minimize": shouldMinimize,
      "minimizer": {
        // It's possible to use esbuild plugin as is as a minimizer.
        // https://github.com/privatenumber/esbuild-loader/blob/e74b94a806c906fbb8fdf877bcc4bc54df8bf213/README.md?plain=1#L183
        // It's also possible to use esbuild plugin together with terser plugin.
        // https://webpack.js.org/plugins/terser-webpack-plugin/#terseroptions
        // It's not clear what is better/right approach, need to investigate.
        switch (shouldMinimize, webpackMinimizer) {
        | (true, EsbuildPlugin) =>
          Some([|makeESBuildPlugin({"target": "es2015"})|])
        | (true, TerserPluginWithEsbuild) =>
          Some([|TerserPlugin.make({"minify": TerserPlugin.esbuildMinify})|])
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
                ->Js.Array.join(~sep="|", _);
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
              let packages = [|"react-helmet"|]->Js.Array.join(~sep="|", _);
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
      switch (webpackDevServerOptions) {
      | None => None
      | Some({listenTo, proxy}) =>
        Some({
          // Prevent Webpack from handling SIGINT and SIGTERM signals
          // because we handle them in our graceful shutdown logic
          "setupExitSignals": false,
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
                renderedPages->Belt.Array.keepMap(page =>
                  switch (page.path) {
                  | Root => None
                  | Path(segments) =>
                    let hasDynamicPart =
                      segments
                      ->Js.Array.find(~f=segment =>
                          segment == PagePath.dynamicSegment
                        , _)
                      ->Belt.Option.isSome;

                    switch (hasDynamicPart) {
                    | false => None
                    | _true =>
                      let pathWithAsterisks =
                        segments
                        ->Js.Array.map(~f=segment =>
                            segment == PagePath.dynamicSegment ? ".*" : segment
                          , _)
                        ->Js.Array.join(~sep="/", _);

                      let regexString = "^/" ++ pathWithAsterisks;

                      let from = Js.Re.fromString(regexString);

                      let to_ =
                        Path.join3(
                          "/",
                          PagePath.toString(page.path),
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
                ->Js.Array.map(~f=(proxy: DevServerOptions.Proxy.t) => {
                    let proxyTo: DevServerOptions.Proxy.devServerProxyTo = {
                      target:
                        switch (proxy.to_.target) {
                        | Host(host) =>
                          DevServerOptions.Proxy.DevServerTarget.makeUnboxed(
                            String(host),
                          )
                        | UnixSocket(socketPath) =>
                          DevServerOptions.Proxy.DevServerTarget.makeUnboxed(
                            Params({
                              host: None,
                              socketPath: Some(socketPath),
                            }),
                          )
                        },
                      pathRewrite:
                        proxy.to_.pathRewrite
                        ->Belt.Option.map(({from, to_}) => {
                            Js.Dict.fromList([(from, to_)])
                          }),
                      secure: proxy.to_.secure,
                      changeOrigin: proxy.to_.changeOrigin,
                      logLevel: "debug",
                    };

                    (proxy.from, proxyTo);
                  }, _)
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
      ~webpackDevServerOptions: option(DevServerOptions.t),
      ~logger: Log.logger,
      ~webpackMode: Mode.t,
      ~webpackMinimizer: Minimizer.t,
      ~globalEnvValues: array((string, string)),
      ~outputDir,
      ~webpackBundleAnalyzerMode: option(WebpackBundleAnalyzerPlugin.Mode.t),
      ~renderedPages: array(RenderedPage.t),
    ) => {
  let config =
    makeConfig(
      ~webpackDevServerOptions,
      ~webpackMode,
      ~logger,
      ~webpackMinimizer,
      ~outputDir,
      ~globalEnvValues,
      ~webpackBundleAnalyzerMode,
      ~renderedPages,
    );
  // TODO handle errors when we make compiler
  let compiler = Webpack.makeCompiler(config);
  (compiler, config);
};

let build =
    (
      ~webpackMode: Mode.t,
      ~webpackMinimizer: Minimizer.t,
      ~logger: Log.logger,
      ~outputDir,
      ~globalEnvValues: array((string, string)),
      ~webpackBundleAnalyzerMode: option(WebpackBundleAnalyzerPlugin.Mode.t),
      ~renderedPages: array(RenderedPage.t),
    )
    : Js.Promise.t(unit) => {
  let durationLabel = "[Webpack.build] duration";
  Js.Console.timeStart(durationLabel);

  logger.info(() => Js.log("[Webpack.build] Building webpack bundle..."));

  let (compiler, _config) =
    makeCompiler(
      ~webpackDevServerOptions=None,
      ~webpackMode,
      ~logger,
      ~outputDir,
      ~webpackMinimizer,
      ~globalEnvValues,
      ~webpackBundleAnalyzerMode: option(WebpackBundleAnalyzerPlugin.Mode.t),
      ~renderedPages,
    );

  Promise.make((~resolve, ~reject as _reject) => {
    compiler->Webpack.run((err, stats) => {
      switch (Js.Nullable.toOption(err)) {
      | Some(error) =>
        logger.info(() => {
          Js.Console.error2("[Webpack.build] Fatal error:", error);
          Process.exit(1);
        })
      | None =>
        logger.info(() => Js.log("[Webpack.build] Success!"));
        switch (Js.Nullable.toOption(stats)) {
        | None =>
          logger.info(() => {
            Js.Console.error("[Webpack.build] Error: stats object is None");
            Process.exit(1);
          })
        | Some(stats) =>
          logger.info(() => Js.log(Webpack.Stats.toString(stats)));

          switch (Webpack.Stats.hasErrors(stats)) {
          | false => ()
          | true =>
            Js.Console.error(
              "[Webpack.build] Error: stats object has errors",
            );
            Process.exit(1);
          };

          switch (Webpack.Stats.hasWarnings(stats)) {
          | false => ()
          | true =>
            logger.info(() => Js.log("[Webpack.build] Stats.hasWarnings"))
          };

          compiler->Webpack.close(closeError => {
            switch (Js.Nullable.toOption(closeError)) {
            | None => Js.Console.timeEnd(durationLabel)
            | Some(error) =>
              logger.info(() =>
                Js.log2("[Webpack.build] Compiler close error:", error)
              )
            };

            let unit = ();
            resolve(. unit);
          });
        };
      }
    })
  });
};

let startDevServer =
    (
      ~webpackDevServerOptions: DevServerOptions.t,
      ~webpackMode: Mode.t,
      ~webpackMinimizer: Minimizer.t,
      ~logger: Log.logger,
      ~outputDir,
      ~globalEnvValues: array((string, string)),
      ~webpackBundleAnalyzerMode: option(WebpackBundleAnalyzerPlugin.Mode.t),
      ~renderedPages: array(RenderedPage.t),
      ~onStart: unit => unit,
    ) => {
  logger.info(() => Js.log("[Webpack] Starting dev server..."));
  let startupDurationLabel = "[Webpack] WebpackDevServer startup duration";
  Js.Console.timeStart(startupDurationLabel);

  let (compiler, config) =
    makeCompiler(
      ~webpackDevServerOptions=Some(webpackDevServerOptions),
      ~webpackMode,
      ~logger,
      ~outputDir,
      ~webpackMinimizer,
      ~globalEnvValues,
      ~webpackBundleAnalyzerMode,
      ~renderedPages,
    );

  let webpackDevServerOptions = config##devServer;

  switch (webpackDevServerOptions) {
  | None =>
    logger.info(() =>
      Js.Console.error(
        "[Webpack] Can't start dev server, config##devServer is None",
      )
    );
    Process.exit(1);
  | Some(webpackDevServerOptions) =>
    let devServer = WebpackDevServer.make(webpackDevServerOptions, compiler);
    devServer->WebpackDevServer.startWithCallback(() => {
      logger.info(() => {
        Js.log("[Webpack] WebpackDevServer started");
        Js.Console.timeEnd(startupDurationLabel);
        onStart();
      })
    });

    GracefulShutdown.addTask(() => {
      Js.log("[Webpack] Stopping dev server...");

      Js.Global.setTimeout(
        () => {
          Js.log("[Webpack] Failed to gracefully shutdown.");
          Process.exit(1);
        },
        GracefulShutdown.gracefulShutdownTimeout,
      )
      ->ignore;

      devServer
      ->WebpackDevServer.stop()
      ->Promise.map(() =>
          Js.log("[Webpack] Dev server stopped successfully")
        );
    });
  };
};
