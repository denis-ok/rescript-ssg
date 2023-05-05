let () = Commands.startWithWorkers(
  ~pages=Pages.pages,
  ~devServerOptions={listenTo: Port(9007), proxy: None},
  ~mode=Development,
  ~outputDir=Pages.outputDir,
  ~logLevel=Info,
  ~globalValues=Pages.globalValues,
  ~webpackBundleAnalyzerMode=None,
  (),
)
