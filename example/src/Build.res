let currentDir = Utils.getDirname()

let () = Commands.build(
  ~pages=Pages.pages,
  ~globalValues=Pages.globalValues,
  ~mode=Production,
  ~outputDir=Pages.outputDir,
  ~logLevel=Info,
  ~compileCommand=Path.join2(currentDir, "../../node_modules/.bin/bsb"),
  ~writeWebpackStatsJson=true,
  ~minimizer=Esbuild,
  (),
)
