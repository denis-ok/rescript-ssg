let currentDir = Utils.getDirname()

let () = Commands.buildWithWorkers(
  ~pages=Pages.pages,
  ~globalValues=Pages.globalValues,
  ~mode=Production,
  ~outputDir=Pages.outputDir,
  ~logLevel=Info,
  ~compileCommand=Path.join2(currentDir, "../../node_modules/.bin/rescript"),
  ~writeWebpackStatsJson=true,
  ~minimizer=TerserPluginWithEsbuild,
  (),
)
