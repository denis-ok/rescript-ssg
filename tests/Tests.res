let dirname = Utils.getDirname()

@val external process: Js.t<'a> = "process"

@module external util: Js.t<'a> = "util"

let inspect = (value): string =>
  util["inspect"](value, {"compact": false, "depth": 20, "colors": true})

let exitWithError = () => process["exit"](1)

let isEqual = (~msg="", v1, v2) =>
  if v1 != v2 {
    Js.log2("Test failed:", msg)
    Js.log2("expected this:", inspect(v1))
    Js.log2("to be equal to:", inspect(v2))
    exitWithError()
  }

module MakeReactAppModuleName = {
  let moduleName = "Page"

  let test = (~pagePath, ~expect) => {
    let reactAppModuleName = PageBuilder.makeReactAppModuleName(~pagePath, ~moduleName)
    isEqual(~msg="makeReactAppModuleName", expect, reactAppModuleName)
  }

  test(~pagePath=".", ~expect="PageApp")

  test(~pagePath="foo/bar", ~expect="foobarPageApp")

  test(~pagePath="foo/bar-baz", ~expect="foobarbazPageApp")

  Js.log("MakeReactAppModuleName tests passed!")
}

module BuildPageHtmlAndReactApp = {
  let dummyLogger: Log.logger = {
    logLevel: Log.Info,
    info: ignore,
    debug: ignore,
  }

  let logger = Log.makeLogger(Info)

  let outputDir = Path.join2(dirname, "output")

  let cleanup = () => Fs.rmSync(outputDir, {force: true, recursive: true})

  let rescriptBinaryPath = Path.join2(dirname, "../node_modules/.bin/rescript")

  let test = page => {
    cleanup()
    PageBuilder.buildPageHtmlAndReactApp(~outputDir, ~logger, page)
    Commands.compileRescript(~rescriptBinaryPath, ~logger, ~logStdoutOnSuccess=false)
    let _ = Fs.readFileSyncAsUtf8(Path.join2(outputDir, "index.html"))->ignore
    let _ = Fs.readFileSyncAsUtf8(Path.join2(outputDir, "TestPageApp.res"))->ignore
  }

  let page: PageBuilder.page = {
    pageWrapper: None,
    component: ComponentWithoutData(<TestPage />),
    modulePath: TestPage.modulePath,
    headCssFilepaths: [],
    path: Root,
  }

  let () = test(page)

  Js.log("BuildPageHtmlAndReactApp tests passed!")
}