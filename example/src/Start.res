let () = Commands.start(
  ~pages=Pages.pages,
  ~devServerOptions={listenTo: Port(9007), proxy: None},
  ~mode=Development,
  ~outputDir=Pages.outputDir,
  ~logLevel=Info,
  ~globalValues=Pages.globalValues,
  ~webpackBundleAnalyzerMode=Some(Server({port: 3234})),
  (),
)
