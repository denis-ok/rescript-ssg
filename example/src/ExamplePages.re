let currentDir = Utils.getDirname();

let pagesOutputDir = Path.join2(currentDir, "../build");

let webpackOutputDir = Path.join2(pagesOutputDir, "bundle");

let makeWrapper = (~arg, ~argReference) =>
  Some(
    PageBuilder.Wrapper1({
      wrapper: ExampleWrapper1.wrapper,
      wrapperReference: ExampleWrapper1.wrapperReference,
      arg,
      argReference,
    }),
  );

let pageIndex = {
  PageBuilder.wrapper: makeWrapper(~arg="123", ~argReference={js|"123"|js}),
  component: <ExampleIndex />,
  moduleName: ExampleIndex.moduleName,
  modulePath: ExampleIndex.modulePath,
  path: ".",
};

let page1 = {
  PageBuilder.wrapper: makeWrapper(~arg="qwe", ~argReference={js|"qwe"|js}),
  component: <ExamplePage1 />,
  moduleName: ExamplePage1.moduleName,
  modulePath: ExamplePage1.modulePath,
  path: "page1",
};

let pages = [pageIndex, page1];

let start = (~mode) =>
  PageBuilder.start(
    ~pages,
    ~outputDir=pagesOutputDir,
    ~webpackOutputDir,
    ~mode,
  );

let build = (~mode) =>
  PageBuilder.build(
    ~pages,
    ~mode,
    ~outputDir=pagesOutputDir,
    ~webpackOutputDir,
    ~rescriptBinaryPath=
      Path.join2(pagesOutputDir, "../../node_modules/.bin/rescript"),
  );
