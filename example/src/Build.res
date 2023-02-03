let currentDir = Utils.getDirname()

let () = Commands.build(
  ~pages=Pages.pages,
  ~globalValues=Pages.globalValues,
  ~mode=Production,
  ~outputDir=Pages.outputDir,
  ~logLevel=Info,
  ~rescriptBinaryPath=Path.join2(currentDir, "../../node_modules/.bin/rescript"),
  ~writeWebpackStatsJson=true,
  ~minimizer=Esbuild,
  (),
)
