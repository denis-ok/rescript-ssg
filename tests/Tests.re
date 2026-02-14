open RescriptSsg;
open NodeTest;

let ( let* ) = (p, f) => Js.Promise.then_(f, p);

module Utils_ = {
  module GetModuleNameFromModulePath = {
    let cases = [|
      "TestPage.bs.js",
      "/TestPage.bs.js",
      "./TestPage.bs.js",
      "/foo/bar/TestPage.bs.js",
      "foo/bar/TestPage.bs.js",
    |];

    let () =
      cases->Belt.Array.forEach(modulePath =>
        NodeTest.test(
          "Utils.getModuleNameFromModulePath: " ++ modulePath,
          _context => {
            let moduleName = Utils.getModuleNameFromModulePath(modulePath);
            expect |> equal(moduleName, "TestPage");
          },
        )
      );
  };
};

module MakeReactAppModuleName = {
  let moduleName = "Page";

  let cases = [|
    (".", "Page__PageApp"),
    ("foo/bar", "foobarPage__PageApp"),
    ("foo/bar-baz", "foobarbazPage__PageApp"),
  |];

  let () =
    cases->Belt.Array.forEach(((pagePath, expected)) =>
      NodeTest.test(
        "PageBuilder.pagePathToPageAppModuleName: " ++ pagePath,
        _context => {
          let reactAppModuleName =
            PageBuilder.pagePathToPageAppModuleName(~pageAppArtifactsSuffix="", ~pagePath, ~moduleName);
          NodeTest.expect |> NodeTest.equal(reactAppModuleName, expected);
        },
      )
    );
};

module BuildPageHtmlAndReactApp = {
  let removeNewlines = (str: string) => {
    let regexp = Js.Re.fromStringWithFlags({js|[\r\n]+|js}, ~flags="g");
    str->Js.String.replaceByRe(~regexp, ~replacement="", _);
  };

  let logger = Log.makeLogger(Info);

  let projectRootDir =
    switch (Js.Dict.get(Process.env, "PROJECT_ROOT_DIR")) {
    | Some(dir) => dir
    | None => Js.Exn.raiseError("PROJECT_ROOT_DIR environment variable not set")
    };

  let outputDir = Path.join2(projectRootDir, "tests/output");

  let artifactsOutputDir = PageBuilder.getArtifactsOutputDir(~outputDir);

  let cleanup = () =>
    Fs.rmSync(
      outputDir,
      {
        force: true,
        recursive: true,
      },
    );

  let compileCommand = "true";

  let runTest = (context, ~name, ~page, ~expectedAppContent, ~expectedHtmlContent as _) =>
    context
    |> TestContext.test(
         name,
         _context => {
           let renderedPage =
             PageBuilder.buildPageHtmlAndReactApp(
               ~melangeArtifactsExtension="mel.mjs",
               ~pageAppArtifactsType=Reason,
               ~outputDir,
               ~melangeOutputDir=None,
               ~logger,
               ~pageAppArtifactsSuffix="",
               page,
             );

           Js.Promise.then_(
             renderedPage => {
               switch (renderedPage) {
               | Error(errors) =>
                 Js.Console.error2("Test failed:", errors);
                 Js.Exn.raiseError("BuildPageHtmlAndReactApp failed");
               | Ok(_) =>
                 Commands.compileRescript(~compileCommand, ~logger);

                 let moduleName = Utils.getModuleNameFromModulePath(page.modulePath);

                 let pagePath: string = page.path->PagePath.toString;

                 let reactAppModuleName =
                   PageBuilder.pagePathToPageAppModuleName(~pageAppArtifactsSuffix="", ~pagePath, ~moduleName);

                 let testPageAppContent =
                   Fs.readFileSyncAsUtf8(Path.join2(artifactsOutputDir, reactAppModuleName ++ ".re"));

                 NodeTest.expect
                 |> NodeTest.equal(removeNewlines(testPageAppContent), removeNewlines(expectedAppContent));

                 let _html = Fs.readFileSyncAsUtf8(Path.join2(artifactsOutputDir, "index.html"));
                 Js.Promise.resolve();
               }
             },
             renderedPage,
           );
         },
       );

  module SimplePage = {
    let page: PageBuilder.page = {
      hydrationMode: FullHydration,
      pageWrapper: None,
      component: ComponentWithoutData(<TestPage />),
      modulePath: TestPage.modulePath,
      headCssFilepaths: [||],
      path: Root,
      globalValues: None,
      headScripts: [||],
      bodyScripts: [||],
    };

    let expectedAppContent = {js|
switch (ReactDOM.querySelector("#root")) {
| Some(root) => ReactDOM.Client.hydrateRoot(root, <TestPage />)->ignore
| None => ()
};
|js};

    let expectedHtmlContent = "";
  };

  module PageWithWrapper = {
    let page: PageBuilder.page = {
      hydrationMode: FullHydration,
      pageWrapper:
        Some({
          component: WrapperWithChildren(children => <TestWrapper> children </TestWrapper>),
          modulePath: TestWrapper.modulePath,
        }),
      component: ComponentWithoutData(<TestPage />),
      modulePath: TestPage.modulePath,
      headCssFilepaths: [||],
      path: Root,
      globalValues: None,
      headScripts: [||],
      bodyScripts: [||],
    };

    let expectedAppContent = {js|
switch (ReactDOM.querySelector("#root")) {
| Some(root) => ReactDOM.Client.hydrateRoot(root, <TestWrapper><TestPage /></TestWrapper>)->ignore
| None => ()
};
|js};
    let expectedHtmlContent = "";
  };

  module PageWithData = {
    let page: PageBuilder.page = {
      hydrationMode: FullHydration,
      pageWrapper: None,
      component:
        ComponentWithData({
          component: data => <TestPageWithData data />,
          data:
            Some({
              bool: true,
              string: "foo",
              int: 1,
              float: 1.23,
              variant: A,
              polyVariant: `hello,
              option: Some("bar"),
            }),
        }),
      modulePath: TestPageWithData.modulePath,
      headCssFilepaths: [||],
      path: Root,
      globalValues: None,
      headScripts: [||],
      bodyScripts: [||],
    };

    let expectedAppContent = {js|
type pageData;
[@mel.module "./TestPageWithData_Data_688ca4c30fca5edb6793.mjs"] external pageData: pageData = "data";

switch (ReactDOM.querySelector("#root")) {
| Some(root) => ReactDOM.Client.hydrateRoot(root, <TestPageWithData data={pageData->Obj.magic} />)->ignore
| None => ()
};
|js};
    let expectedHtmlContent = "";
  };

  module PageWrapperWithDataAndPageWithData = {
    let page: PageBuilder.page = {
      hydrationMode: FullHydration,
      pageWrapper:
        Some({
          component:
            WrapperWithDataAndChildren({
              component: (data, children) => <TestWrapperWithData data> children </TestWrapperWithData>,
              data:
                Some({
                  bool: true,
                  string: "foo",
                  int: 1,
                  float: 1.23,
                  variant: A,
                  polyVariant: `hello,
                  option: Some("bar"),
                }),
            }),
          modulePath: TestWrapperWithData.modulePath,
        }),
      component:
        ComponentWithData({
          component: data => <TestPageWithData data />,
          data:
            Some({
              bool: true,
              string: "foo",
              int: 1,
              float: 1.23,
              variant: A,
              polyVariant: `hello,
              option: Some("bar"),
            }),
        }),
      modulePath: TestPageWithData.modulePath,
      headCssFilepaths: [||],
      path: Root,
      globalValues: None,
      headScripts: [||],
      bodyScripts: [||],
    };

    let expectedAppContent = {js|
type pageWrapperData;
[@mel.module "./__pageWrappersData/TestWrapperWithData_Data_688ca4c30fca5edb6793.mjs"] external pageWrapperData: pageWrapperData = "data";

type pageData;
[@mel.module "./TestPageWithData_Data_688ca4c30fca5edb6793.mjs"] external pageData: pageData = "data";

switch (ReactDOM.querySelector("#root")) {
| Some(root) => ReactDOM.Client.hydrateRoot(root, <TestWrapperWithData data={pageWrapperData->Obj.magic} ><TestPageWithData data={pageData->Obj.magic} /></TestWrapperWithData>)->ignore
| None => ()
};
|js};
    let expectedHtmlContent = "";
  };

  let () =
    NodeTest.Promise.testWithOptions(
      "BuildPageHtmlAndReactApp",
      NodeTest.makeOptions(),
      context => {
        context
        |> TestContext.afterEach(_context => {
             cleanup();
             Js.Promise.resolve();
           });

        let* () =
          runTest(
            context,
            ~name="SimplePage",
            ~page=SimplePage.page,
            ~expectedAppContent=SimplePage.expectedAppContent,
            ~expectedHtmlContent=SimplePage.expectedHtmlContent,
          );

        let* () =
          runTest(
            context,
            ~name="PageWithWrapper",
            ~page=PageWithWrapper.page,
            ~expectedAppContent=PageWithWrapper.expectedAppContent,
            ~expectedHtmlContent=PageWithWrapper.expectedHtmlContent,
          );

        let* () =
          runTest(
            context,
            ~name="PageWithData",
            ~page=PageWithData.page,
            ~expectedAppContent=PageWithData.expectedAppContent,
            ~expectedHtmlContent=PageWithData.expectedHtmlContent,
          );

        let* () =
          runTest(
            context,
            ~name="PageWrapperWithDataAndPageWithData",
            ~page=PageWrapperWithDataAndPageWithData.page,
            ~expectedAppContent=PageWrapperWithDataAndPageWithData.expectedAppContent,
            ~expectedHtmlContent=PageWrapperWithDataAndPageWithData.expectedHtmlContent,
          );

        Js.Promise.resolve();
      },
    )
    |> ignore;
};
