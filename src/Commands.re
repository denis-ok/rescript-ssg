let build =
    (
      ~outputDir: string,
      ~webpackOutputDir: string,
      ~rescriptBinaryPath: string,
      ~logSetting: Log.level,
      ~mode: Webpack.Mode.t,
      ~pages: array(PageBuilder.page),
    ) => {
  PageBuilder.buildPages(~outputDir, ~logSetting, pages);

  Js.log("[PageBuilder.build] Compiling React app files...");

  switch (ChildProcess.execSync(. rescriptBinaryPath, {"encoding": "utf8"})) {
  | exception (Js.Exn.Error(error)) =>
    Js.log2(
      "[PageBuilder.build] Rescript build failed:\n",
      error->Js.Exn.message,
    );
    Js.log2(
      "[PageBuilder.build] Rescript build failed:\n",
      error->ChildProcess.Error.stdout,
    );
    Process.exit(1);
  | stdout => Js.log2("[PageBuilder.build] Rescript build success:\n", stdout)
  };

  Js.log("[PageBuilder.build] Building webpack bundle...");

  Webpack.build(~mode, ~webpackOutputDir, ~verbose=true);
};

let start =
    (
      ~outputDir: string,
      ~webpackOutputDir: string,
      ~mode: Webpack.Mode.t,
      ~logSetting: Log.level,
      ~pages: array(PageBuilder.page),
    ) => {
  PageBuilder.buildPages(~outputDir, ~logSetting, pages);

  Watcher.startWatcher(~outputDir, ~logSetting, pages);

  Webpack.startDevServer(~mode, ~logSetting, ~webpackOutputDir);
};
