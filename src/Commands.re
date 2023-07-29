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
  let logger = Log.makeLogger(logLevel);

  let webpackPages =
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

  webpackPages
  ->Promise.map(webpackPages => {
      let () = compileRescript(~compileCommand, ~logger);

      switch (Bundler.bundler) {
      | Esbuild =>
        logger.info(() =>
          Js.log("[rescript-ssg][build] Bundling with Esbuild...")
        );

        let () =
          Esbuild.build(~outputDir, ~webpackPages)
          ->Promise.map(_ =>
              logger.info(() =>
                Js.log("[rescript-ssg][build] Bundling finished!")
              )
            )
          ->ignore;

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
            ~webpackPages,
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
      ~buildWorkersCount: option(int)=?,
      (),
    ) => {
  let logger = Log.makeLogger(logLevel);

  let webpackPages =
    BuildPageWorkerHelpers.buildPagesWithWorkers(
      ~pages,
      ~outputDir,
      ~melangeOutputDir,
      ~logger,
      ~globalEnvValues,
      ~buildWorkersCount,
      ~exitOnPageBuildError=true,
      ~generatedFilesSuffix="",
    );

  webpackPages
  ->Promise.map(webpackPages => {
      let () =
        Webpack.startDevServer(
          ~devServerOptions,
          ~webpackBundleAnalyzerMode,
          ~mode,
          ~logger,
          ~outputDir,
          ~minimizer,
          ~globalEnvValues,
          ~webpackPages,
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
