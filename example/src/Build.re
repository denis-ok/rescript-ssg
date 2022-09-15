let () =
  PageBuilder.build(
    ~pages=Pages.pages,
    ~mode=Production,
    ~outputDir=Pages.pagesOutputDir,
    ~webpackOutputDir=Pages.webpackOutputDir,
    ~rescriptBinaryPath=
      Path.join2(Pages.pagesOutputDir, "../../node_modules/.bin/rescript"),
  );
