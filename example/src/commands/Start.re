let currentDir = Utils.getDirname();

let () =
  Commands.start(
    ~pageAppArtifact=Js,
    ~pages=Pages.pages,
    ~webpackDevServerOptions={listenTo: Port(9007), proxy: None},
    ~webpackMode=Development,
    ~outputDir=Pages.outputDir,
    ~projectRootDir=Path.join2(currentDir, "../../../"),
    ~logLevel=Info,
    ~globalEnvValues=Pages.globalEnvValues,
    ~webpackBundleAnalyzerMode=None,
    ~buildWorkersCount=1,
    (),
  );
