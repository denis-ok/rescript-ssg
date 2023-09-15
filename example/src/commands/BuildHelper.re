open Ssg;

let currentDir = Utils.getDirname();

let build = (~minimizer) =>
  Commands.build(
    ~pages=Pages.pages,
    ~globalEnvValues=Pages.globalEnvValues,
    ~mode=Production,
    ~outputDir=Pages.outputDir,
    ~projectRootDir=Path.join2(currentDir, "../../../"),
    ~logLevel=Info,
    ~compileCommand="make build",
    ~melangeOutputDir=Pages.melangeOutputDir,
    ~minimizer,
    ~webpackBundleAnalyzerMode=
      Some(Static({reportHtmlFilepath: "webpack-bundle/index.html"})),
    ~buildWorkersCount=1,
    ~generatedFilesSuffix=UnixTimestamp,
    (),
  );