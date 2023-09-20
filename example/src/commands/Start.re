open Ssg;

let currentDir = Utils.getDirname();

let () =
  Commands.start(
    ~pages=Pages.pages,
    ~devServerOptions={listenTo: Port(9007), proxy: None},
    ~mode=Development,
    ~outputDir=Pages.outputDir,
    // This is correct path by luck, it should be fixed.
    ~projectRootDir=Path.join2(currentDir, "../../../"),
    ~melangeOutputDir=Pages.melangeOutputDir,
    ~logLevel=Info,
    ~globalEnvValues=Pages.globalEnvValues,
    ~webpackBundleAnalyzerMode=None,
    ~buildWorkersCount=1,
    (),
  );
