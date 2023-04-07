let compileRescript = (~compileCommand: string, ~logger: Log.logger) => {
  let durationLabel = "[Commands.compileRescript] duration";
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
  | Ok () =>
    logger.info(() => {Js.log("[Commands.compileRescript] Success!")});
    Js.Console.timeEnd(durationLabel);
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

let build =
    (
      ~outputDir: string,
      ~compileCommand: string,
      ~logLevel: Log.level,
      ~mode: Webpack.Mode.t,
      ~pages: array(PageBuilder.page),
      ~writeWebpackStatsJson=false,
      ~minimizer: Webpack.Minimizer.t=Terser,
      ~globalValues: array((string, string))=[||],
      (),
    ) => {
  let () = GlobalValues.unsafeAdd(globalValues);

  let logger = Log.makeLogger(logLevel);

  let webpackPages = PageBuilder.buildPages(~outputDir, ~logger, pages);

  let () = compileRescript(~compileCommand, ~logger);

  let () =
    Webpack.build(
      ~mode,
      ~outputDir,
      ~logger,
      ~writeWebpackStatsJson,
      ~minimizer,
      ~globalValues,
      ~webpackPages,
    );

  ();
};

let buildWithWorkers =
    (
      ~outputDir: string,
      ~compileCommand: string,
      ~logLevel: Log.level,
      ~mode: Webpack.Mode.t,
      ~pages: array(PageBuilder.page),
      ~writeWebpackStatsJson=false,
      ~minimizer: Webpack.Minimizer.t=Terser,
      ~globalValues: array((string, string))=[||],
      (),
    ) => {
  let () = GlobalValues.unsafeAdd(globalValues);

  let logger = Log.makeLogger(logLevel);

  let rebuildPages =
    pages->Js.Array2.map(page =>
      RebuildPageWorkerHelpers.mapPageToPageForRebuild(~page, ~outputDir)
    );

  let workerDatas =
    rebuildPages
    ->Js.Array2.map(page => {
        let workerData: RebuildPageWorkerT.workerData = {
          pages: [|page|],
          logLevel: logger.logLevel,
          globalValues,
        };
        workerData;
      })
    ->Array.splitIntoChunks(~chunkSize=2);

  let createPageCallbacks =
    workerDatas->Js.Array2.map((workerDatas, ()) =>
      workerDatas
      ->Js.Array2.map(workerData =>
          RebuildPageWorkerHelpers.runRebuildPageWorker(
            ~workerData, ~onExit=_exitCode =>
            ()
          )
        )
      ->Js.Promise.all
      ->Promise.map(result => Array.flat1(result))
    );

  let webpackPages =
    Promise.seqRun(createPageCallbacks)
    ->Js.Promise.all
    ->Promise.map(result => Array.flat1(result));

  webpackPages
  ->Promise.map(webpackPages => {
      let () = compileRescript(~compileCommand, ~logger);
      let () =
        Webpack.build(
          ~mode,
          ~outputDir,
          ~logger,
          ~writeWebpackStatsJson,
          ~minimizer,
          ~globalValues,
          ~webpackPages,
        );
      ();
    })
  ->ignore;
};

let start =
    (
      ~outputDir: string,
      ~mode: Webpack.Mode.t,
      ~logLevel: Log.level,
      ~pages: array(PageBuilder.page),
      ~devServerOptions: Webpack.DevServerOptions.t,
      ~minimizer: Webpack.Minimizer.t=Terser,
      ~globalValues: array((string, string))=[||],
      (),
    ) => {
  let () = GlobalValues.unsafeAdd(globalValues);

  let logger = Log.makeLogger(logLevel);

  let webpackPages = PageBuilder.buildPages(~outputDir, ~logger, pages);

  let () =
    Webpack.startDevServer(
      ~devServerOptions,
      ~mode,
      ~logger,
      ~outputDir,
      ~minimizer,
      ~globalValues,
      ~webpackPages,
    );

  let () = Watcher.startWatcher(~outputDir, ~logger, ~globalValues, pages);

  ();
};
