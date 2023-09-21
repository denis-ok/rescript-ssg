let build = (~minimizer) =>
  Commands.build(
    ~pages=Pages.pages,
    ~globalEnvValues=Pages.globalEnvValues,
    ~mode=Production,
    ~outputDir=Pages.outputDir,
    ~projectRootDir=Pages.projectRootDir,
    ~logLevel=Info,
    ~compileCommand=
      Path.join2(Pages.projectRootDir, "node_modules/.bin/rescript"),
    ~minimizer,
    ~webpackBundleAnalyzerMode=
      Some(Static({reportHtmlFilepath: "webpack-bundle/index.html"})),
    ~buildWorkersCount=1,
    ~generatedFilesSuffix=UnixTimestamp,
    (),
  );
