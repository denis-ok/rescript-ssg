let build = () =>
  PageBuilder.build(
    ~pages=ExamplePages.pages,
    ~mode=Production,
    ~outputDir=ExamplePages.pagesOutputDir,
    ~webpackOutputDir=ExamplePages.webpackOutputDir,
    ~rescriptBinaryPath=
      Path.join2(
        ExamplePages.pagesOutputDir,
        "../../node_modules/.bin/rescript",
      ),
  );
