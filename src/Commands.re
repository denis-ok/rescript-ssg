let compileRescript =
    (
      ~rescriptBinaryPath: string,
      ~logger: Log.logger,
      ~logStdoutOnSuccess: bool,
    ) => {
  switch (ChildProcess.execSync(. rescriptBinaryPath, {"encoding": "utf8"})) {
  | exception (Js.Exn.Error(error)) =>
    logger.info(() => {
      Js.Console.error2(
        "[Commands.build] Rescript build failed:\n",
        error->Js.Exn.message,
      );
      Js.Console.error2(
        "[Commands.build] Rescript build failed:\n",
        error->ChildProcess.Error.stdout,
      );
    });

    Process.exit(1);
  | stdout =>
    if (logStdoutOnSuccess) {
      logger.info(() =>
        Js.log2("[PageBuilder.build] Rescript build success:\n", stdout)
      );
    }
  };
};

let build =
    (
      ~outputDir: string,
      ~rescriptBinaryPath: string,
      ~logLevel: Log.level,
      ~mode: Webpack.Mode.t,
      ~pages: array(PageBuilder.page),
    ) => {
  let logger = Log.makeLogger(logLevel);

  PageBuilder.buildPages(~outputDir, ~logger, pages);

  logger.info(() =>
    Js.log("[Commands.build] Compiling fresh React app files...")
  );

  let () =
    compileRescript(~rescriptBinaryPath, ~logger, ~logStdoutOnSuccess=true);

  Webpack.build(~mode, ~outputDir, ~logger);
};

let start =
    (
      ~outputDir: string,
      ~mode: Webpack.Mode.t,
      ~logLevel: Log.level,
      ~pages: array(PageBuilder.page),
      ~devServerOptions: Webpack.DevServerOptions.t
    ) => {
  let logger = Log.makeLogger(logLevel);

  PageBuilder.buildPages(~outputDir, ~logger, pages);

  Watcher.startWatcher(~outputDir, ~logger, pages);

  Webpack.startDevServer(~devServerOptions, ~mode, ~logger, ~outputDir);
};
