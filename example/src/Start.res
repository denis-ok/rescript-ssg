let () = Commands.start(
  ~devServerOptions={listenTo: Port(9007), proxy: None},
  ~mode=Development,
  ~outputDir=Pages.outputDir,
  ~logLevel=Info,
  ~minimizer=Terser,
  ~pages=Pages.pages,
  (),
)
