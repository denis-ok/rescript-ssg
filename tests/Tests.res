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

let isIncludes = (~msg="", ~string: string, ~substring: string) =>
  if !(string->Js.String2.includes(substring)) {
    Js.log2("Test failed:", msg)
    Js.log2("expected string:", string)
    Js.log2("to include substring:", substring)
    exitWithError()
  }

module Utils_ = {
  module GetModuleNameFromModulePath = {
    let testName = "Utils.getModuleNameFromModulePath"
    let test = modulePath => {
      let moduleName = Utils.getModuleNameFromModulePath(modulePath)
      isEqual(~msg=testName, moduleName, "TestPage")
    }
    test("TestPage.bs.js")
    test("/TestPage.bs.js")
    test("./TestPage.bs.js")
    test("/foo/bar/TestPage.bs.js")
    test("foo/bar/TestPage.bs.js")
    Js.log2(testName, " tests passed!")
  }
}

module MakeReactAppModuleName = {
  let moduleName = "Page"

  let test = (~pagePath, ~expect) => {
    let reactAppModuleName = PageBuilder.makeReactAppModuleName(~pagePath, ~moduleName)
    isEqual(~msg="makeReactAppModuleName", reactAppModuleName, expect)
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

  let removeMultipleNewlines = (str: string) => {
    let regex = Js.Re.fromStringWithFlags(`[\r\n]+`, ~flags="g")
    str->Js.String2.replaceByRe(regex, "")
  }

  let logger = Log.makeLogger(Info)

  let outputDir = Path.join2(dirname, "output")

  let intermediateFilesOutputDir = PageBuilder.getIntermediateFilesOutputDir(~outputDir)

  let cleanup = () => Fs.rmSync(outputDir, {force: true, recursive: true})

  let rescriptBinaryPath = Path.join2(dirname, "../node_modules/.bin/rescript")

  let test = (~page, ~expectedAppContent, ~expectedHtmlContent as _) => {
    cleanup()

    PageBuilder.buildPageHtmlAndReactApp(~outputDir, ~logger, page)

    Commands.compileRescript(~rescriptBinaryPath, ~logger, ~logStdoutOnSuccess=false)

    let moduleName = Utils.getModuleNameFromModulePath(page.modulePath)

    let pagePath: string = page.path->PageBuilderT.PagePath.toString

    let reactAppModuleName = PageBuilder.makeReactAppModuleName(~pagePath, ~moduleName)

    let testPageAppContent = Fs.readFileSyncAsUtf8(
      Path.join2(intermediateFilesOutputDir, reactAppModuleName ++ ".res"),
    )

    isEqual(removeMultipleNewlines(testPageAppContent), removeMultipleNewlines(expectedAppContent))

    let _html = Fs.readFileSyncAsUtf8(Path.join2(intermediateFilesOutputDir, "index.html"))
  }

  module SimplePage = {
    let page: PageBuilder.page = {
      pageWrapper: None,
      component: ComponentWithoutData(<TestPage />),
      modulePath: TestPage.modulePath,
      headCssFilepaths: [],
      path: Root,
    }

    let expectedAppContent = `
switch ReactDOM.querySelector("#root") {
| Some(root) => ReactDOM.hydrate(<TestPage />, root)
| None => ()
}
`
    let expectedHtmlContent = ``

    let () = test(~page, ~expectedAppContent, ~expectedHtmlContent)
  }

  module PageWithData = {
    let page: PageBuilder.page = {
      pageWrapper: None,
      component: ComponentWithData({
        component: data => <TestPageWithData data />,
        data: Some({
          bool: true,
          string: "foo",
          int: 1,
          float: 1.23,
          variant: A,
          polyVariant: #hello,
          option: Some("bar"),
        }),
      }),
      modulePath: TestPageWithData.modulePath,
      headCssFilepaths: [],
      path: Root,
    }

    let expectedAppContent = `
@module("./TestPageWithDataData.js") external pageData: string = "data"

switch ReactDOM.querySelector("#root") {
| Some(root) => ReactDOM.hydrate(<TestPageWithData data={pageData->Js.Json.parseExn->Obj.magic} />, root)
| None => ()
}
`
    let expectedHtmlContent = ``

    let () = test(~page, ~expectedAppContent, ~expectedHtmlContent)
  }

  Js.log("BuildPageHtmlAndReactApp tests passed!")
}
