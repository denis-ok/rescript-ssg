let currentDir = Utils.getDirname();

let pagesOutputDir = Path.join2(currentDir, "../build");

let webpackOutputDir = Path.join2(pagesOutputDir, "bundle");

let pageIndex: PageBuilder.page = {
  component: <ExampleIndex />,
  moduleName: ExampleIndex.moduleName,
  modulePath: ExampleIndex.modulePath,
  path: ".",
};

let page1: PageBuilder.page = {
  component: <ExamplePage1 />,
  moduleName: ExamplePage1.moduleName,
  modulePath: ExamplePage1.modulePath,
  path: "page1",
};

let buildPageWithWrapper1 = (~arg, ~argReference, page) => {
  let wrapper =
    PageBuilder.Wrapper1({
      wrapper: ExampleWrapper1.wrapper,
      wrapperReference: ExampleWrapper1.wrapperReference,
      arg,
      argReference,
    });

  PageBuilder.buildPageHtmlAndReactApp(
    ~outputDir=pagesOutputDir,
    ~wrapper,
    page,
  );
};

let buildPageFiles = () => {
  buildPageWithWrapper1(pageIndex, ~arg="123", ~argReference={js|"123"|js});
  buildPageWithWrapper1(page1, ~arg="qwe", ~argReference={js|"qwe"|js});
};

let start = (~mode) => PageBuilder.start(~mode, ~webpackOutputDir);

let build = (~mode) =>
  PageBuilder.build(
    ~mode,
    ~webpackOutputDir,
    ~rescriptBinaryPath=
      Path.join2(pagesOutputDir, "../../node_modules/.bin/rescript"),
  );
