let currentDir = Utils.getDirname()

let () = Commands.build(
  ~pages=Pages.pages,
  ~globalEnvValues=Pages.globalEnvValues,
  ~mode=Production,
  ~outputDir=Pages.outputDir,
  ~logLevel=Info,
  ~compileCommand=Path.join2(currentDir, "../../node_modules/.bin/rescript"),
  ~minimizer=TerserPluginWithEsbuild,
  ~webpackBundleAnalyzerMode=Some(Static({reportHtmlFilepath: "webpack-bundle/index.html"})),
  // buildWorkersCount=1 makes pages build sequental to make console output readable
  ~buildWorkersCount=1,
  (),
)
