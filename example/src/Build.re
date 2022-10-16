let () =
  Commands.build(
    ~mode=Production,
    ~outputDir=Pages.pagesOutputDir,
    ~webpackOutputDir=Pages.webpackOutputDir,
    ~logSetting=Info,
    ~rescriptBinaryPath=
      Path.join2(Pages.pagesOutputDir, "../../node_modules/.bin/rescript"),
    ~pages=Pages.pages,
  );
