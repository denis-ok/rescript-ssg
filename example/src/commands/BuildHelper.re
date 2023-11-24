let build = (~webpackMinimizer) =>
  Commands.build(
    ~pageAppArtifactsType=Js,
    ~pages=Pages.pages,
    ~globalEnvValues=Pages.globalEnvValues,
    ~webpackMode=Production,
    ~outputDir=Pages.outputDir,
    ~projectRootDir=Pages.projectRootDir,
    ~logLevel=Info,
    ~compileCommand=
      Path.join2(Pages.projectRootDir, "node_modules/.bin/rescript"),
    ~webpackMinimizer,
    ~webpackBundleAnalyzerMode=
      Some(Static({reportHtmlFilepath: "webpack-bundle/index.html"})),
    ~buildWorkersCount=1,
    ~pageAppArtifactsSuffix=UnixTimestamp,
    (),
  )
  ->Promise.map(_ => Js.log("[rescript-ssg] Build success!"))
  ->ignore;
