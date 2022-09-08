let () =
  PageBuilder.start(
    ~devServerOptions={listenTo: Port(9007), proxy: None},
    ~mode=Development,
    ~pages=ExamplePages.pages,
    ~outputDir=ExamplePages.pagesOutputDir,
    ~webpackOutputDir=ExamplePages.webpackOutputDir,
  );
