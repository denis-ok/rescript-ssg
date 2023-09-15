open Ssg;

let () =
  Commands.start(
    ~pages=Pages.pages,
    ~devServerOptions={listenTo: Port(9007), proxy: None},
    ~mode=Development,
    ~outputDir=Pages.outputDir,
    ~melangeOutputDir=Pages.melangeOutputDir,
    ~logLevel=Info,
    ~globalEnvValues=Pages.globalEnvValues,
    ~webpackBundleAnalyzerMode=None,
    ~buildWorkersCount=1,
    (),
  );
