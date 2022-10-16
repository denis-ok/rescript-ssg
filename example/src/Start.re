let () =
  Commands.start(
    ~devServerOptions={listenTo: Port(9007), proxy: None},
    ~mode=Development,
    ~outputDir=Pages.pagesOutputDir,
    ~webpackOutputDir=Pages.webpackOutputDir,
    ~logSetting=Info,
    ~pages=Pages.pages,
  );
