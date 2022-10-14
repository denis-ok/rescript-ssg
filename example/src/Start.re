let () =
  Commands.start(
    ~devServerOptions={listenTo: Port(9007), proxy: None},
    ~mode=Development,
    ~pages=Pages.pages,
    ~outputDir=Pages.pagesOutputDir,
    ~webpackOutputDir=Pages.webpackOutputDir,
  );
