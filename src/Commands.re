let compileRescript = (~rescriptBinaryPath: string, ~logger: Log.logger) => {
  let durationLabel = "[Commands.compileRescript] duration";
  Js.Console.timeStart(durationLabel);

  logger.info(() =>
    Js.log("[Commands.compileRescript] Compiling fresh React app files...")
  );

  switch (
    ChildProcess.spawnSync(
      rescriptBinaryPath,
      [||],
      {"encoding": "utf8", "stdio": "inherit"},
    )
  ) {
  | exitCode when exitCode == 0 =>
    logger.info(() => {Js.log("[Commands.compileRescript] Success!")});
    Js.Console.timeEnd(durationLabel);
  | exitCode =>
    logger.info(() => {
      Js.Console.error2(
        "[Commands.compileRescript] Failure! Exit code:",
        exitCode,
      )
    });
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
      ~rescriptBinaryPath: string,
      ~logLevel: Log.level,
      ~mode: Webpack.Mode.t,
      ~pages: array(PageBuilder.page),
      ~writeWebpackStatsJson=false,
      ~minimizer: Webpack.Minimizer.t=Terser,
      (),
    ) => {
  let logger = Log.makeLogger(logLevel);

  PageBuilder.buildPages(~outputDir, ~logger, pages);

  let () = compileRescript(~rescriptBinaryPath, ~logger);

  Webpack.build(
    ~mode,
    ~outputDir,
    ~logger,
    ~writeWebpackStatsJson,
    ~minimizer,
  );
};

let start =
    (
      ~outputDir: string,
      ~mode: Webpack.Mode.t,
      ~logLevel: Log.level,
      ~pages: array(PageBuilder.page),
      ~devServerOptions: Webpack.DevServerOptions.t,
      ~minimizer: Webpack.Minimizer.t=Terser,
      (),
    ) => {
  let logger = Log.makeLogger(logLevel);

  PageBuilder.buildPages(~outputDir, ~logger, pages);

  Watcher.startWatcher(~outputDir, ~logger, pages);

  Webpack.startDevServer(
    ~devServerOptions,
    ~mode,
    ~logger,
    ~outputDir,
    ~minimizer,
  );
};
