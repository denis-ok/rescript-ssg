type esbuild;

type context;

type buildResult = {
  errors: array(Js.Json.t),
  warnings: array(Js.Json.t),
  metafile: Js.Json.t,
};

module Plugin = {
  // https://esbuild.github.io/plugins/#on-start

  type buildCallbacks = {
    onStart: (unit => unit) => unit,
    onEnd: (buildResult => unit) => unit,
  };

  type t = {
    name: string,
    setup: buildCallbacks => unit,
  };

  let watchModePlugin = {
    name: "watchPlugin",
    setup: buildCallbacks => {
      buildCallbacks.onEnd(_buildResult =>
        Js.log("[Esbuild] Rebuild finished!")
      );
    },
  };
};

[@module "esbuild"] external esbuild: esbuild = "default";

[@bs.send]
external build': (esbuild, Js.t('a)) => Promise.t(buildResult) = "build";

[@send]
external context: (esbuild, Js.t('a)) => Promise.t(context) = "context";

[@send] external watch: (context, unit) => Promise.t(unit) = "watch";

[@send] external dispose: (context, unit) => Promise.t(unit) = "dispose";

// https://esbuild.github.io/api/#serve-arguments
type serveOptions = {
  port: int,
  servedir: option(string),
};

// https://esbuild.github.io/api/#serve-return-values
type serveResult = {
  host: string,
  port: int,
};

[@send]
external serve: (context, serveOptions) => Promise.t(serveResult) = "serve";

module HtmlPlugin = {
  // https://github.com/craftamap/esbuild-plugin-html/blob/b74debfe7f089a4f073f5a0cf9bbdb2e59370a7c/src/index.ts#L8
  type options = {files: array(htmlFileConfiguration)}
  and htmlFileConfiguration = {
    filename: string,
    entryPoints: array(string),
    htmlTemplate: string,
    scriptLoading: string,
  };

  [@mel.module "@craftamap/esbuild-plugin-html"]
  external make: (. options) => Plugin.t = "htmlPlugin";
};

module LogLevel = {
  // https://esbuild.github.io/api/#log-level
  type t =
    | Silent
    | Error
    | Warning
    | Info
    | Debug;

  let toString = (t: t) =>
    switch (t) {
    | Silent => "silent"
    | Error => "error"
    | Warning => "warning"
    | Info => "info"
    | Debug => "debug"
    };
};

let makeConfig =
    (
      ~mode: Bundler.mode,
      ~outputDir: string,
      ~projectRootDir: string,
      ~globalEnvValues: array((string, string)),
      ~renderedPages: array(RenderedPage.t),
      ~logOverride: Js.Dict.t(LogLevel.t),
      ~logLevel: LogLevel.t,
      ~logLimit: int,
    ) =>
  // https://esbuild.github.io/api/
  {
    "entryPoints": renderedPages->Js.Array2.map(page => page.entryPath),
    "entryNames": Bundler.assetsDirname ++ "/" ++ "js/[dir]/[name]-[hash]",
    "chunkNames": Bundler.assetsDirname ++ "/" ++ "js/_chunks/[name]-[hash]",
    "assetNames": Bundler.assetsDirname ++ "/" ++ "[name]-[hash]",
    "outdir": Bundler.getOutputDir(~outputDir),
    "publicPath": Bundler.assetPrefix,
    "format": "esm",
    "bundle": true,
    "minify": {
      switch (mode) {
      | Build => true
      | Watch => false
      };
    },
    "metafile": true,
    "splitting": true,
    "treeShaking": true,
    "logLimit": logLimit,
    "logLevel": logLevel->LogLevel.toString,
    "logOverride": {
      let logOverride: Js.Dict.t(string) =
        logOverride
        ->Js.Dict.entries
        ->Js.Array2.map(((error, logLevel)) =>
            (error, logLevel->LogLevel.toString)
          )
        ->Js.Dict.fromArray;
      logOverride;
    },
    "define": Bundler.getGlobalEnvValuesDict(globalEnvValues),
    "loader": {
      Bundler.assetFileExtensionsWithoutCss
      ->Js.Array2.map(ext => {("." ++ ext, "file")})
      ->Js.Dict.fromArray;
    },
    "plugins": {
      let htmlPluginFiles =
        renderedPages->Js.Array2.map(renderedPage => {
          let pagePath = renderedPage.path->PagePath.toString;

          // entryPoint must be relative path to the root of user's project
          let entryPathRelativeToProjectRoot =
            Path.relative(~from=projectRootDir, ~to_=renderedPage.entryPath);

          {
            // filename field, which if actually a path will be relative to "outdir".
            HtmlPlugin.filename: pagePath ++ "/index.html",
            entryPoints: [|entryPathRelativeToProjectRoot|],
            htmlTemplate: renderedPage.htmlTemplatePath,
            scriptLoading: "module",
          };
        });

      let htmlPlugin = HtmlPlugin.make(. {files: htmlPluginFiles});

      switch (mode) {
      | Build => [|htmlPlugin|]
      | Watch => [|htmlPlugin, Plugin.watchModePlugin|]
      };
    },
  };

let build =
    (
      ~outputDir: string,
      ~projectRootDir: string,
      ~globalEnvValues: array((string, string)),
      ~renderedPages: array(RenderedPage.t),
      ~logLevel: LogLevel.t=Warning,
      ~logOverride: Js.Dict.t(LogLevel.t)=Js.Dict.empty(),
      (),
    ) => {
  Js.log("[Esbuild] Bundling...");
  let durationLabel = "[Esbuild] Success! Duration";
  Js.Console.timeStart(durationLabel);

  let config =
    makeConfig(
      ~mode=Build,
      ~outputDir,
      ~projectRootDir,
      ~globalEnvValues,
      ~renderedPages,
      ~logLevel,
      ~logOverride,
      ~logLimit=10,
    );

  esbuild
  ->build'(config)
  ->Promise.map(_buildResult => {
      // let json =
      //   Js.Json.stringifyAny(_buildResult.metafile)
      //   ->Belt.Option.getWithDefault("");
      // Fs.writeFileSync(~path=Path.join2(outputDir, "meta.json"), ~data=json);
      Js.Console.timeEnd(
        durationLabel,
      )
    })
  ->Promise.catch(error => {
      Js.Console.error2(
        "[Esbuild] Build failed! Promise.catch:",
        error->Util.inspect,
      );
      Process.exit(1);
    });
};

let watchAndServe =
    (
      ~outputDir,
      ~projectRootDir: string,
      ~globalEnvValues: array((string, string)),
      ~renderedPages: array(RenderedPage.t),
      ~port: int,
      ~logLevel: LogLevel.t=Warning,
      ~logOverride: Js.Dict.t(LogLevel.t)=Js.Dict.empty(),
      ~logLimit=10,
      (),
    )
    : Promise.t(serveResult) => {
  let config =
    makeConfig(
      ~mode=Watch,
      ~outputDir,
      ~projectRootDir,
      ~globalEnvValues,
      ~renderedPages,
      ~logLevel,
      ~logOverride,
      ~logLimit,
    );
  Js.log("[Esbuild] Starting esbuild...");
  let watchDurationLabel = "[Esbuild] Watch mode started! Duration";
  let serveDurationLabel = "[Esbuild] Serve mode started! Duration";
  Js.Console.timeStart(watchDurationLabel);

  let contextPromise = esbuild->context(config);

  GracefulShutdown.addTask(() => {
    Js.log("[Esbuild] Stopping esbuild...");

    Js.Global.setTimeout(
      () => {
        Js.log("[Esbuild] Failed to gracefully shutdown.");
        Process.exit(1);
      },
      GracefulShutdown.gracefulShutdownTimeout,
    )
    ->ignore;

    contextPromise
    ->Promise.flatMap(context => context->dispose())
    ->Promise.map(() => Js.log("[Esbuild] Stopped successfully"));
  });

  contextPromise
  ->Promise.flatMap(context => context->watch())
  ->Promise.map(() => Js.Console.timeEnd(watchDurationLabel))
  ->Promise.catch(error => {
      Js.Console.error2("[Esbuild] Failed to start watch mode:", error);
      Process.exit(1);
    })
  ->Promise.flatMap(() => {
      Js.Console.timeStart(serveDurationLabel);
      contextPromise->Promise.flatMap(context =>
        context->serve({port, servedir: Some(config##outdir)})
      );
    })
  ->Promise.map(serveResult => {
      Js.Console.timeEnd(serveDurationLabel);
      serveResult;
    })
  ->Promise.catch(error => {
      Js.Console.error2("[Esbuild] Failed to start serve mode:", error);
      Process.exit(1);
    });
};

let subscribeToRebuildEventScript = "new EventSource('/esbuild').addEventListener('change', () => location.reload());";

type import = {
  path: string,
  kind: string,
  original: string,
};

type input = {
  bytes: int,
  imports: array(import),
  format: string,
};

type metafile = {inputs: Js.Dict.t(input)};

external unsafeJsonToMetafile: Js.Json.t => metafile = "%identity";

let getModuleDependencies =
    (~exitOnError, ~projectRootDir: string, ~modulePath: string)
    : Js.Promise.t(array(string)) => {
  let config = {
    "entryPoints": [|modulePath|],
    // Outdir technically isn't used because "write" is false, but esbuild has complaints without it
    "outdir": "unused",
    "write": false,
    "format": "esm",
    "bundle": true,
    "minify": false,
    "metafile": true,
    "splitting": false,
    "treeShaking": false,
    "logLevel": LogLevel.toString(Silent),
    "loader": {
      Bundler.assetFileExtensionsWithoutCss
      ->Js.Array2.map(ext => {("." ++ ext, "file")})
      ->Js.Dict.fromArray;
    },
  };

  esbuild
  ->build'(config)
  ->Promise.map(buildResult => {
      // let json =
      //   Js.Json.stringifyAny(buildResult.metafile)
      //   ->Belt.Option.getWithDefault("");
      // Fs.writeFileSync(~path="meta.json", ~data=json);
      let metafile = buildResult.metafile->unsafeJsonToMetafile;
      let dependencies =
        metafile.inputs
        ->Js.Dict.entries
        ->Belt.Array.keepMap(((dependencyPath, _)) => {
            // Filter out dependencies from node_modules
            switch (dependencyPath->Js.String2.indexOf("node_modules")) {
            | (-1) => Some(Path.join2(projectRootDir, dependencyPath))
            | _ => None
            }
          });
      dependencies;
    })
  ->Promise.catch(error =>
      if (exitOnError) {
        Js.Console.error2(
          "[Esbuild] Get module dependencies failed! Stopping process. Promise.catch:",
          error->Util.inspect,
        );
        Process.exit(1);
      } else {
        Js.Console.error2(
          "[Esbuild] Get module dependencies failed! Returning empty array. Promise.catch:",
          error->Util.inspect,
        );
        Promise.resolve([||]);
      }
    );
};
