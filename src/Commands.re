let checkDuplicatedPagePaths = (pages: array(array(PageBuilder.page))) => {
  Js.log("[rescript-ssg] Checking duplicated page paths...");

  let pagesDict = Js.Dict.empty();

  pages->Js.Array2.forEach(pages' => {
    pages'->Js.Array2.forEach(page => {
      let pagePath = PagePath.toString(page.path);
      switch (pagesDict->Js.Dict.get(pagePath)) {
      | None => pagesDict->Js.Dict.set(pagePath, page)
      | Some(_) =>
        Js.Console.error2(
          "[rescript-ssg] List of pages contains pages with the same paths. Duplicated page path:",
          pagePath,
        );
        Process.exit(1);
      };
    })
  });
};

let compileRescript = (~compileCommand: string, ~logger: Log.logger) => {
  let durationLabel = "[Commands.compileRescript] Success! Duration";
  Js.Console.timeStart(durationLabel);

  logger.info(() =>
    Js.log("[Commands.compileRescript] Compiling fresh React app files...")
  );

  switch (
    ChildProcess.spawnSync(
      compileCommand,
      [||],
      {"shell": true, "encoding": "utf8", "stdio": "inherit"},
    )
  ) {
  | Ok () => logger.info(() => Js.Console.timeEnd(durationLabel))
  | Error(JsError(error)) =>
    logger.info(() => {
      Js.Console.error2("[Commands.compileRescript] Failure! Error:", error)
    });
    Process.exit(1);
  | Error(ExitCodeIsNotZero(exitCode)) =>
    Js.Console.error2(
      "[Commands.compileRescript] Failure! Exit code is not zero:",
      exitCode,
    );
    Process.exit(1);
  | exception (Js.Exn.Error(error)) =>
    logger.info(() => {
      Js.Console.error2(
        "[Commands.compileRescript] Exception:\n",
        error->Js.Exn.message,
      )
    });
    Process.exit(1);
  };
};

type generatedFilesSuffix =
  | NoSuffix
  | UnixTimestamp;

let initializeAndBuildPages =
    (
      ~pageAppArtifactsType: PageBuilder.pageAppArtifactsType,
      ~logLevel,
      ~buildWorkersCount,
      ~pages: array(array(PageBuilder.page)),
      ~outputDir,
      ~melangeOutputDir,
      ~globalEnvValues,
      ~generatedFilesSuffix,
      ~bundlerMode: Bundler.mode,
    ) => {
  let () = checkDuplicatedPagePaths(pages);

  let logger = Log.makeLogger(logLevel);

  let pages =
    switch (Bundler.bundler, bundlerMode) {
    | (Esbuild, Watch) =>
      pages->Js.Array2.map(pages =>
        pages->Js.Array2.map(page =>
          {
            ...page,
            // Add a script to implement live reloading with esbuild
            // https://esbuild.github.io/api/#live-reload
            headScripts:
              Js.Array2.concat(
                [|Esbuild.subscribeToRebuildEventScript|],
                page.headScripts,
              ),
          }
        )
      )
    | _ => pages
    };

  let renderedPages =
    BuildPageWorkerHelpers.buildPagesWithWorkers(
      ~pageAppArtifactsType,
      ~buildWorkersCount,
      ~pages,
      ~outputDir,
      ~melangeOutputDir,
      ~logger,
      ~globalEnvValues,
      ~exitOnPageBuildError=true,
      ~generatedFilesSuffix=
        switch (generatedFilesSuffix) {
        | NoSuffix => ""
        | UnixTimestamp =>
          "_" ++ Js.Date.make()->Js.Date.valueOf->Belt.Float.toString
        },
    );

  (logger, pages, renderedPages);
};

let build =
    (
      ~pages: array(array(PageBuilder.page)),
      ~globalEnvValues: array((string, string))=[||],
      ~pageAppArtifactsType: PageBuilder.pageAppArtifactsType=Reason,
      ~generatedFilesSuffix: generatedFilesSuffix=UnixTimestamp,
      ~projectRootDir: string,
      ~outputDir: string,
      ~melangeOutputDir: option(string)=?,
      ~compileCommand: string,
      ~logLevel: Log.level,
      ~buildWorkersCount: option(int)=?,
      ~webpackMode: Webpack.Mode.t,
      ~webpackMinimizer: Webpack.Minimizer.t=Terser,
      ~webpackBundleAnalyzerMode:
         option(Webpack.WebpackBundleAnalyzerPlugin.Mode.t)=None,
      ~esbuildLogLevel: option(Esbuild.LogLevel.t)=?,
      ~esbuildLogOverride: option(Js.Dict.t(Esbuild.LogLevel.t))=?,
      (),
    ) => {
  let (logger, _pages, renderedPages) =
    initializeAndBuildPages(
      ~pageAppArtifactsType,
      ~logLevel,
      ~buildWorkersCount,
      ~pages,
      ~outputDir,
      ~melangeOutputDir,
      ~globalEnvValues,
      ~generatedFilesSuffix,
      ~bundlerMode=Build,
    );

  renderedPages
  ->Promise.map(renderedPages => {
      let () =
        switch (pageAppArtifactsType) {
        | Reason => compileRescript(~compileCommand, ~logger)
        | Js => ()
        };

      switch (Bundler.bundler) {
      | Esbuild =>
        let () =
          Esbuild.build(
            ~outputDir,
            ~projectRootDir,
            ~globalEnvValues,
            ~renderedPages,
            ~logLevel=?esbuildLogLevel,
            ~logOverride=?esbuildLogOverride,
            (),
          )
          ->ignore;
        ();
      | Webpack =>
        let () =
          Webpack.build(
            ~webpackMode,
            ~outputDir,
            ~logger,
            ~webpackBundleAnalyzerMode,
            ~webpackMinimizer,
            ~globalEnvValues,
            ~renderedPages,
          );
        ();
      };
    })
  ->ignore;
};

let start =
    (
      ~pages: array(array(PageBuilder.page)),
      ~globalEnvValues: array((string, string))=[||],
      ~pageAppArtifactsType: PageBuilder.pageAppArtifactsType=Reason,
      ~generatedFilesSuffix: generatedFilesSuffix=UnixTimestamp,
      ~projectRootDir: string,
      ~outputDir: string,
      ~melangeOutputDir: option(string)=?,
      ~logLevel: Log.level,
      ~buildWorkersCount: option(int)=?,
      ~webpackMode: Webpack.Mode.t,
      ~webpackMinimizer: Webpack.Minimizer.t=Terser,
      ~webpackBundleAnalyzerMode:
         option(Webpack.WebpackBundleAnalyzerPlugin.Mode.t)=None,
      ~webpackDevServerOptions: Webpack.DevServerOptions.t,
      ~esbuildLogLevel: option(Esbuild.LogLevel.t)=?,
      ~esbuildLogOverride: option(Js.Dict.t(Esbuild.LogLevel.t))=?,
      ~esbuildLogLimit: option(int)=?,
      ~esbuildMainServerPort: int=8010,
      ~esbuildProxyServerPort: int=8011,
      ~esbuildProxyRules: array(ProxyServer.ProxyRule.t)=[||],
      (),
    ) => {
  let (logger, pages, renderedPages) =
    initializeAndBuildPages(
      ~pageAppArtifactsType,
      ~logLevel,
      ~buildWorkersCount,
      ~pages,
      ~outputDir,
      ~melangeOutputDir,
      ~globalEnvValues,
      ~generatedFilesSuffix,
      ~bundlerMode=Watch,
    );

  let startFileWatcher = (): unit =>
    FileWatcher.startWatcher(
      ~projectRootDir,
      ~pageAppArtifactsType,
      ~outputDir,
      ~melangeOutputDir,
      ~logger,
      ~globalEnvValues,
      pages,
    );

  let delayBeforeDevServerStart =
    switch (pageAppArtifactsType) {
    | Js => 100
    | Reason =>
      // A compilation most likely is still in progress after reason artifacts emitted,
      // starting dev server + file watcher after a little delay.
      2000
    };

  renderedPages
  ->Promise.map(renderedPages => {
      Js.Global.setTimeout(
        () => {
          switch (Bundler.bundler) {
          | Esbuild =>
            Esbuild.watchAndServe(
              ~outputDir,
              ~projectRootDir,
              ~globalEnvValues,
              ~renderedPages,
              ~port=esbuildMainServerPort,
              ~logLevel=?esbuildLogLevel,
              ~logOverride=?esbuildLogOverride,
              ~logLimit=?esbuildLogLimit,
              (),
            )
            ->Promise.map(serveResult => {
                let () =
                  ProxyServer.start(
                    ~port=esbuildProxyServerPort,
                    ~targetHost=serveResult.host,
                    ~targetPort=serveResult.port,
                    ~proxyRules=esbuildProxyRules,
                    ~pagePaths=
                      renderedPages->Js.Array2.map(page =>
                        PagePath.toString(page.path)
                        ->Utils.maybeAddSlashPrefix
                        ->Utils.maybeAddSlashSuffix
                      ),
                  );
                let () = startFileWatcher();
                ();
              })
            ->ignore
          | Webpack =>
            let () =
              Webpack.startDevServer(
                ~webpackDevServerOptions,
                ~webpackBundleAnalyzerMode,
                ~webpackMode,
                ~logger,
                ~outputDir,
                ~webpackMinimizer,
                ~globalEnvValues,
                ~renderedPages,
                ~onStart=startFileWatcher,
              );
            ();
          }
        },
        delayBeforeDevServerStart,
      )
      ->ignore
    })
  ->ignore;
};
