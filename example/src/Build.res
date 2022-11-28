let currentDir = Utils.getDirname()

let () = Commands.build(
  ~mode=Production,
  ~outputDir=Pages.outputDir,
  ~logLevel=Info,
  ~rescriptBinaryPath=Path.join2(currentDir, "../../node_modules/.bin/bsb"),
  ~pages=Pages.pages,
  ~writeWebpackStatsJson=true,
)
