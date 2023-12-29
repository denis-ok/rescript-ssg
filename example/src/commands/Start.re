open Ssg;

let currentDir = Utils.getDirname();

let () =
  Commands.start(
    ~pageAppArtifactsType=Js,
    ~pages=Pages.pages,
    ~webpackDevServerOptions={listenTo: Port(9007), proxy: None},
    ~webpackMode=Development,
    ~outputDir=Pages.outputDir,
    ~projectRootDir=Pages.projectRootDir,
    ~melangeOutputDir=Pages.melangeOutputDir,
    ~logLevel=Info,
    ~globalEnvValues=Pages.globalEnvValues,
    ~webpackBundleAnalyzerMode=None,
    ~buildWorkersCount=1,
    (),
  );
