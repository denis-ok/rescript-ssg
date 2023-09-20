open Ssg;

let build = (~minimizer) =>
  Commands.build(
    ~pages=Pages.pages,
    ~globalEnvValues=Pages.globalEnvValues,
    ~mode=Production,
    ~outputDir=Pages.outputDir,
    ~projectRootDir=Pages.projectRoot,
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
