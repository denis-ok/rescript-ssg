type esbuild;

type plugin;

type context;

type buildResult = {
  errors: array(Js.Json.t),
  warnings: array(Js.Json.t),
  metafile: Js.Json.t,
};

[@bs.module "esbuild"] external esbuild: esbuild = "default";

[@bs.send]
external build: (esbuild, Js.t('a)) => Promise.t(buildResult) = "build";

[@send]
external context: (esbuild, Js.t('a)) => Promise.t(context) = "context";

[@send] external watch: (context, unit) => Promise.t(unit) = "watch";

[@send] external dispose: (context, unit) => Promise.t(unit) = "dispose";

module HtmlPlugin = {
  // https://github.com/craftamap/esbuild-plugin-html/blob/b74debfe7f089a4f073f5a0cf9bbdb2e59370a7c/src/index.ts#L8
  type options = {files: array(htmlFileConfiguration)}
  and htmlFileConfiguration = {
    filename: string,
    entryPoints: array(string),
    htmlTemplate: string,
    scriptLoading: string,
  };

  [@bs.module "@craftamap/esbuild-plugin-html"]
  external make: (. options) => plugin = "htmlPlugin";
};

let makeConfig =
    (
            ~mode: Bundler.mode,
      ~outputDir: string,
      ~projectRootDir: string,
      ~globalEnvValues: array((string, string)),
      ~renderedPages: array(RenderedPage.t),
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
    "logLevel": "warning",
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

      [|htmlPlugin|];
    },
  };

let build =
    (
      ~outputDir: string,
      ~projectRootDir: string,
      ~globalEnvValues: array((string, string)),
      ~renderedPages: array(RenderedPage.t),
    ) => {
  Js.log("[Esbuild.build] Bundling...");
  let durationLabel = "[Esbuild.build] Success! Duration";
  Js.Console.timeStart(durationLabel);

  let config =
    makeConfig(~mode=Build, ~outputDir, ~projectRootDir, ~globalEnvValues, ~renderedPages);

  esbuild
  ->build(config)
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
        "[Esbuild.build] Build failed! Promise.catch:",
        error->Util.inspect,
      );
      Process.exit(1);
    });
};

let watch =
    (
      ~outputDir,
      ~globalEnvValues: array((string, string)),
      ~renderedPages: array(RenderedPage.t),
    ) => {
  Js.log("[Esbuild.watch] Starting watch mode...");
  let durationLabel = "[Esbuild.watch] Watcher started! Duration";
  Js.Console.timeStart(durationLabel);

  let config =
    makeConfig(~mode=Watch, ~outputDir, ~globalEnvValues, ~renderedPages);

  let contextPromise = esbuild->context(config);

  let () =
    contextPromise
    ->Promise.flatMap(context => context->watch())
    ->Promise.map(() => {
        Js.Console.timeEnd(durationLabel);
        Js.log("[Esbuild.watch] Starting watch mode...");
      })
    ->Promise.catch(error => {
        Js.Console.error2("[Esbuild.watch] Promise.catch:", error);
        Process.exit(1);
      })
    ->ignore;

  ();
};
