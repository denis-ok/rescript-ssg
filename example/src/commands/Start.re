open Ssg;

let currentDir = Utils.getDirname();

let () =
  Commands.start(
    ~pageAppArtifactsType=Js,
    ~pages=Pages.pages,
    ~globalEnvValues=Pages.globalEnvValues,
    ~webpackMode=Development,
    ~outputDir=Pages.outputDir,
    ~melangeOutputDir=Pages.melangeOutputDir,
    ~projectRootDir=Pages.projectRootDir,
    ~logLevel=Info,
    ~webpackDevServerOptions={listenTo: Port(9007), proxy: None},
    ~webpackBundleAnalyzerMode=None,
    ~buildWorkersCount=1,
    ~pageAppArtifactsSuffix=UnixTimestamp,
    (),
  );
