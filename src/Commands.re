let checkDuplicatedPagePaths = (pages: array(array(PageBuilder.page))) => {
  Js.log("[rescript-ssg] Checking duplicated page paths...");

  let pagesDict = Js.Dict.empty();

  pages->Js.Array.forEach(~f=pages' => {
    pages'->Js.Array.forEach(~f=(page: PageBuilder.page) => {
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
    }, _)
  }, _);
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

type pageAppArtifactsSuffix =
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
      ~pageAppArtifactsSuffix,
      ~bundlerMode: Bundler.mode,
    ) => {
  let () = checkDuplicatedPagePaths(pages);

  let logger = Log.makeLogger(logLevel);

  let pages =
    switch (Bundler.bundler, bundlerMode) {
    | (Esbuild, Watch) =>
      pages->Js.Array.map(~f=pages =>
        pages->Js.Array.map(~f=(page: PageBuilder.page) =>
          {
            ...page,
            // Add a script to implement live reloading with esbuild
            // https://esbuild.github.io/api/#live-reload
            headScripts:
              Js.Array.concat(
                ~other=page.headScripts,
                [|Esbuild.subscribeToRebuildEventScript|]
              ),
          }
        , _)
      , _)
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
      ~pageAppArtifactsSuffix=
        switch (pageAppArtifactsSuffix) {
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
      ~pageAppArtifactsSuffix: pageAppArtifactsSuffix=UnixTimestamp,
      ~projectRootDir: string,
      ~outputDir: string,
      ~melangeOutputDir: option(string)=?,
      ~compileCommand: option(string)=?,
      ~logLevel: Log.level,
      ~buildWorkersCount: option(int)=?,
      ~webpackMode: Webpack.Mode.t=Production,
      ~webpackMinimizer: Webpack.Minimizer.t=Terser,
      ~webpackBundleAnalyzerMode:
         option(Webpack.WebpackBundleAnalyzerPlugin.Mode.t)=None,
      ~esbuildLogLevel: option(Esbuild.LogLevel.t)=?,
      ~esbuildLogOverride: option(Js.Dict.t(Esbuild.LogLevel.t))=?,
      (),
    )
    : Js.Promise.t(unit) => {
  let (logger, _pages, renderedPages) =
    initializeAndBuildPages(
      ~pageAppArtifactsType,
      ~logLevel,
      ~buildWorkersCount,
      ~pages,
      ~outputDir,
      ~melangeOutputDir,
      ~globalEnvValues,
      ~pageAppArtifactsSuffix,
      ~bundlerMode=Build,
    );

  renderedPages->Promise.flatMap(renderedPages => {
    let () =
      switch (pageAppArtifactsType, compileCommand) {
      | (Reason, None) =>
        Js.Console.error(
          "[Commands.build] Error: missing compileCommand param for Reason artifacts",
        );
        Process.exit(1);
      | (Reason, Some(compileCommand)) =>
        compileRescript(~compileCommand, ~logger)
      | (Js, _) => ()
      };

    switch (Bundler.bundler) {
    | Esbuild =>
      Esbuild.build(
        ~outputDir,
        ~projectRootDir,
        ~globalEnvValues,
        ~renderedPages,
        ~logLevel=?esbuildLogLevel,
        ~logOverride=?esbuildLogOverride,
        (),
      )
    | Webpack =>
      Webpack.build(
        ~webpackMode,
        ~outputDir,
        ~logger,
        ~webpackBundleAnalyzerMode,
        ~webpackMinimizer,
        ~globalEnvValues,
        ~renderedPages,
      )
    };
  });
};

let start =
    (
      ~pages: array(array(PageBuilder.page)),
      ~globalEnvValues: array((string, string))=[||],
      ~pageAppArtifactsType: PageBuilder.pageAppArtifactsType=Reason,
      ~pageAppArtifactsSuffix: pageAppArtifactsSuffix=UnixTimestamp,
      ~projectRootDir: string,
      ~outputDir: string,
      ~melangeOutputDir: option(string)=?,
      ~logLevel: Log.level,
      ~buildWorkersCount: option(int)=?,
      ~webpackMode: Webpack.Mode.t=Development,
      ~webpackMinimizer: Webpack.Minimizer.t=Terser,
      ~webpackBundleAnalyzerMode:
         option(Webpack.WebpackBundleAnalyzerPlugin.Mode.t)=None,
      ~webpackDevServerOptions: Webpack.DevServerOptions.t={
                                                             listenTo:
                                                               Port(9000),
                                                             proxy: None,
                                                           },
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
      ~pageAppArtifactsSuffix,
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
        ~f=() => {
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
                      renderedPages->Js.Array.map(~f=(page: RenderedPage.t) =>
                        PagePath.toString(page.path)
                        ->Utils.maybeAddSlashPrefix
                        ->Utils.maybeAddSlashSuffix
                      , _),
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
