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

  let testCase = (~pagePath, ~expect as expected) => {
    NodeTest.Promise.subtest(
      "PageBuilder.pagePathToPageAppModuleName: " ++ pagePath,
      () => {
        let reactAppModuleName =
          PageBuilder.pagePathToPageAppModuleName(~pageAppArtifactsSuffix="", ~pagePath, ~moduleName);
        NodeTest.expect |> NodeTest.equal(reactAppModuleName, expected);
        Js.Promise.resolve();
      },
    );
  };

  let run = () => {
    let* () = testCase(~pagePath=".", ~expect="Page__PageApp");
    let* () = testCase(~pagePath="foo/bar", ~expect="foobarPage__PageApp");
    let* () = testCase(~pagePath="foo/bar-baz", ~expect="foobarbazPage__PageApp");
    Js.Promise.resolve();
  };
};

module BuildPageHtmlAndReactApp = {
  let removeNewlines = (str: string) => {
    let regexp = Js.Re.fromStringWithFlags({js|[\r\n]+|js}, ~flags="g");
    str->Js.String.replaceByRe(~regexp, ~replacement="", _);
  };

  let logger = Log.makeLogger(Info);

  let projectRootDir = "/Users/denstr/projects/rescript-ssg";

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

  let runTest = (~name, ~page, ~expectedAppContent, ~expectedHtmlContent as _) =>
    NodeTest.Promise.subtest(
      name,
      () => {
        cleanup();

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

  let run = () => {
    let* () =
      runTest(
        ~name="BuildPageHtmlAndReactApp: SimplePage",
        ~page=SimplePage.page,
        ~expectedAppContent=SimplePage.expectedAppContent,
        ~expectedHtmlContent=SimplePage.expectedHtmlContent,
      );

    let* () =
      runTest(
        ~name="BuildPageHtmlAndReactApp: PageWithWrapper",
        ~page=PageWithWrapper.page,
        ~expectedAppContent=PageWithWrapper.expectedAppContent,
        ~expectedHtmlContent=PageWithWrapper.expectedHtmlContent,
      );

    let* () =
      runTest(
        ~name="BuildPageHtmlAndReactApp: PageWithData",
        ~page=PageWithData.page,
        ~expectedAppContent=PageWithData.expectedAppContent,
        ~expectedHtmlContent=PageWithData.expectedHtmlContent,
      );

    let* () =
      runTest(
        ~name="BuildPageHtmlAndReactApp: PageWrapperWithDataAndPageWithData",
        ~page=PageWrapperWithDataAndPageWithData.page,
        ~expectedAppContent=PageWrapperWithDataAndPageWithData.expectedAppContent,
        ~expectedHtmlContent=PageWrapperWithDataAndPageWithData.expectedHtmlContent,
      );

    Js.Promise.resolve();
  };
};

NodeTest.Promise.test(
  "RescriptSsg Tests",
  _context => {
    let* () = MakeReactAppModuleName.run();
    let* () = BuildPageHtmlAndReactApp.run();
    Js.Promise.resolve();
  },
);
