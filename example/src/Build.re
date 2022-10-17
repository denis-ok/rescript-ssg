let () =
  Commands.build(
    ~mode=Production,
    ~outputDir=Pages.pagesOutputDir,
    ~webpackOutputDir=Pages.webpackOutputDir,
    ~logLevel=Info,
    ~rescriptBinaryPath=
      Path.join2(Pages.pagesOutputDir, "../../node_modules/.bin/rescript"),
    ~pages=Pages.pages,
  );
