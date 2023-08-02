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
    let reactAppModuleName = PageBuilder.pagePathToPageAppModuleName(
      ~generatedFilesSuffix="",
      ~pagePath,
      ~moduleName,
    )
    isEqual(~msg="makeReactAppModuleName", reactAppModuleName, expect)
  }

  test(~pagePath=".", ~expect="Page__PageApp")

  test(~pagePath="foo/bar", ~expect="foobarPage__PageApp")

  test(~pagePath="foo/bar-baz", ~expect="foobarbazPage__PageApp")

  Js.log("MakeReactAppModuleName tests passed!")
}

module BuildPageHtmlAndReactApp = {
  let dummyLogger: Log.logger = {
    logLevel: Log.Info,
    info: ignore,
    debug: ignore,
  }

  let removeNewlines = (str: string) => {
    let regex = Js.Re.fromStringWithFlags(`[\r\n]+`, ~flags="g")
    str->Js.String2.replaceByRe(regex, "")
  }

  let logger = Log.makeLogger(Info)

  let outputDir = Path.join2(dirname, "output")

  let artifactsOutputDir = PageBuilder.getArtifactsOutputDir(~outputDir)

  let cleanup = () => Fs.rmSync(outputDir, {force: true, recursive: true})

  let compileCommand = Path.join2(dirname, "../node_modules/.bin/rescript")

  let test = (~page, ~expectedAppContent, ~expectedHtmlContent as _) => {
    cleanup()

    let renderedPage = PageBuilder.buildPageHtmlAndReactApp(
      ~outputDir,
      ~melangeOutputDir=None,
      ~logger,
      ~generatedFilesSuffix="",
      page,
    )

    renderedPage->Promise.map(renderedPage => {
      switch renderedPage {
      | Error(errors) =>
        Js.Console.error2("Test failed:", errors)
        Process.exit(1)
      | Ok(_) =>
        Commands.compileRescript(~compileCommand, ~logger)

        let moduleName = Utils.getModuleNameFromModulePath(page.modulePath)

        let pagePath: string = page.path->PageBuilderT.PagePath.toString

        let reactAppModuleName = PageBuilder.pagePathToPageAppModuleName(
          ~generatedFilesSuffix="",
          ~pagePath,
          ~moduleName,
        )

        let testPageAppContent = Fs.readFileSyncAsUtf8(
          Path.join2(artifactsOutputDir, reactAppModuleName ++ ".res"),
        )

        isEqual(removeNewlines(testPageAppContent), removeNewlines(expectedAppContent))

        let _html = Fs.readFileSyncAsUtf8(Path.join2(artifactsOutputDir, "index.html"))
      }
    })
  }

  module SimplePage = {
    let page: PageBuilder.page = {
      hydrationMode: FullHydration,
      pageWrapper: None,
      component: ComponentWithoutData(<TestPage />),
      modulePath: TestPage.modulePath,
      headCssFilepaths: [],
      path: Root,
      globalValues: None,
      headScripts: [],
      bodyScripts: [],
    }

    let expectedAppContent = `
switch ReactDOM.querySelector("#root") {
| Some(root) => ReactDOM.hydrate(<TestPage />, root)
| None => ()
}
`
    let expectedHtmlContent = ``

    let testPromise = () => test(~page, ~expectedAppContent, ~expectedHtmlContent)
  }

  module PageWithWrapper = {
    let page: PageBuilder.page = {
      hydrationMode: FullHydration,
      pageWrapper: Some({
        component: WrapperWithChildren(children => <TestWrapper> children </TestWrapper>),
        modulePath: TestWrapper.modulePath,
      }),
      component: ComponentWithoutData(<TestPage />),
      modulePath: TestPage.modulePath,
      headCssFilepaths: [],
      path: Root,
      globalValues: None,
      headScripts: [],
      bodyScripts: [],
    }

    let expectedAppContent = `
switch ReactDOM.querySelector("#root") {
| Some(root) => ReactDOM.hydrate(<TestWrapper><TestPage /></TestWrapper>, root)
| None => ()
}
`
    let expectedHtmlContent = ``

    let testPromise = () => test(~page, ~expectedAppContent, ~expectedHtmlContent)
  }

  module PageWithData = {
    let page: PageBuilder.page = {
      hydrationMode: FullHydration,
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
      globalValues: None,
      headScripts: [],
      bodyScripts: [],
    }

    let expectedAppContent = `
type pageData
@module("./TestPageWithData_Data_688ca4c30fca5edb6793.js") external pageData: pageData = "data"

switch ReactDOM.querySelector("#root") {
| Some(root) => ReactDOM.hydrate(<TestPageWithData data={pageData->Obj.magic} />, root)
| None => ()
}
`
    let expectedHtmlContent = ``

    let testPromise = () => test(~page, ~expectedAppContent, ~expectedHtmlContent)
  }

  module PageWrapperWithDataAndPageWithData = {
    let page: PageBuilder.page = {
      hydrationMode: FullHydration,
      pageWrapper: Some({
        component: WrapperWithDataAndChildren({
          component: (data, children) => <TestWrapperWithData data> children </TestWrapperWithData>,
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
        modulePath: TestWrapperWithData.modulePath,
      }),
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
      globalValues: None,
      headScripts: [],
      bodyScripts: [],
    }

    let expectedAppContent = `
type pageWrapperData
@module("./__pageWrappersData/TestWrapperWithData_Data_688ca4c30fca5edb6793.js") external pageWrapperData: pageWrapperData = "data"
type pageData
@module("./TestPageWithData_Data_688ca4c30fca5edb6793.js") external pageData: pageData = "data"

switch ReactDOM.querySelector("#root") {
| Some(root) => ReactDOM.hydrate(
<TestWrapperWithData data={pageWrapperData->Obj.magic} >
<TestPageWithData data={pageData->Obj.magic} />
</TestWrapperWithData>, root)
| None => ()
}
`
    let expectedHtmlContent = ``

    let testPromise = () => test(~page, ~expectedAppContent, ~expectedHtmlContent)
  }

  let tests =
    [
      SimplePage.testPromise,
      PageWithWrapper.testPromise,
      PageWithData.testPromise,
      PageWrapperWithDataAndPageWithData.testPromise,
    ]
    ->Promise.seqRun
    ->Promise.map(_ => Js.log("BuildPageHtmlAndReactApp tests passed!"))
    ->ignore
}
