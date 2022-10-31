let () = Commands.build(
  ~mode=Production,
  ~outputDir=Pages.outputDir,
  ~logLevel=Info,
  ~rescriptBinaryPath=Path.join2(Pages.outputDir, "../../node_modules/.bin/rescript"),
  ~pages=Pages.pages,
)
