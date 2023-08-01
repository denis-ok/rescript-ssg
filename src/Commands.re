let checkDuplicatedPagePaths = (pages: array(PageBuilder.page)) => {
  Js.log("[rescript-ssg] Checking duplicated page paths...");

  let pagesDict = Js.Dict.empty();

  pages->Js.Array2.forEach(page => {
    let pagePath = PageBuilderT.PagePath.toString(page.path);
    switch (pagesDict->Js.Dict.get(pagePath)) {
    | None => pagesDict->Js.Dict.set(pagePath, page)
    | Some(_) =>
      Js.Console.error2(
        "[rescript-ssg] List of pages contains pages with the same paths. Duplicated page path:",
        pagePath,
      );
      Process.exit(1);
    };
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
      ~logLevel,
      ~buildWorkersCount,
      ~pages,
      ~outputDir,
      ~melangeOutputDir,
      ~globalEnvValues,
      ~generatedFilesSuffix,
    ) => {
  let () = checkDuplicatedPagePaths(pages);

  let logger = Log.makeLogger(logLevel);

  let renderedPages =
    BuildPageWorkerHelpers.buildPagesWithWorkers(
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

  (logger, renderedPages);
};

let build =
    (
      ~outputDir: string,
      ~melangeOutputDir: option(string)=?,
      ~compileCommand: string,
      ~logLevel: Log.level,
      ~mode: Webpack.Mode.t,
      ~pages: array(PageBuilder.page),
      ~webpackBundleAnalyzerMode:
         option(Webpack.WebpackBundleAnalyzerPlugin.Mode.t)=None,
      ~minimizer: Webpack.Minimizer.t=Terser,
      ~globalEnvValues: array((string, string))=[||],
      ~generatedFilesSuffix: generatedFilesSuffix=UnixTimestamp,
      ~buildWorkersCount: option(int)=?,
      (),
    ) => {
  let (logger, renderedPages) =
    initializeAndBuildPages(
      ~logLevel,
      ~buildWorkersCount,
      ~pages,
      ~outputDir,
      ~melangeOutputDir,
      ~globalEnvValues,
      ~generatedFilesSuffix,
    );

  renderedPages
  ->Promise.map(renderedPages => {
      let () = compileRescript(~compileCommand, ~logger);

      switch (Bundler.bundler) {
      | Esbuild =>
        let () =
          Esbuild.build(~outputDir, ~globalEnvValues, ~renderedPages)->ignore;
        ();
      | Webpack =>
        let () =
          Webpack.build(
            ~mode,
            ~outputDir,
            ~logger,
            ~webpackBundleAnalyzerMode,
            ~minimizer,
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
      ~outputDir: string,
      ~melangeOutputDir: option(string)=?,
      ~mode: Webpack.Mode.t,
      ~logLevel: Log.level,
      ~pages: array(PageBuilder.page),
      ~devServerOptions: Webpack.DevServerOptions.t,
      ~webpackBundleAnalyzerMode:
         option(Webpack.WebpackBundleAnalyzerPlugin.Mode.t),
      ~minimizer: Webpack.Minimizer.t=Terser,
      ~globalEnvValues: array((string, string))=[||],
      ~generatedFilesSuffix: generatedFilesSuffix=UnixTimestamp,
      ~buildWorkersCount: option(int)=?,
      (),
    ) => {
  let (logger, renderedPages) =
    initializeAndBuildPages(
      ~logLevel,
      ~buildWorkersCount,
      ~pages,
      ~outputDir,
      ~melangeOutputDir,
      ~globalEnvValues,
      ~generatedFilesSuffix,
    );

  renderedPages
  ->Promise.map(renderedPages => {
      let () =
        Webpack.startDevServer(
          ~devServerOptions,
          ~webpackBundleAnalyzerMode,
          ~mode,
          ~logger,
          ~outputDir,
          ~minimizer,
          ~globalEnvValues,
          ~renderedPages,
        );
      ();
    })
  ->ignore;

  let () =
    Watcher.startWatcher(
      ~outputDir,
      ~melangeOutputDir,
      ~logger,
      ~globalEnvValues,
      pages,
    );

  ();
};
