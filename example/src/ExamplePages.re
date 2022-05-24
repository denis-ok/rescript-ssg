let currentDir = Utils.getDirname();

let pagesOutputDir = Path.join2(currentDir, "../build");

let webpackOutputDir = Path.join2(pagesOutputDir, "bundle");

let () = PageBuilder.setOutputDir(pagesOutputDir);

let pageIndex: PageBuilder.page = {
  component: <ExampleIndex />,
  moduleName: ExampleIndex.moduleName,
  modulePath: ExampleIndex.modulePath,
  slug: "index",
  path: ".",
};

let page1: PageBuilder.page = {
  component: <ExamplePage1 />,
  moduleName: ExamplePage1.moduleName,
  modulePath: ExamplePage1.modulePath,
  slug: "page1",
  path: "page1",
};

let buildPageWithWrapper1' =
    (~wrapper, ~wrapperReference, ~arg, ~argReference, ~page) => {
  let wrapper =
    PageBuilder.Wrapper1({wrapper, wrapperReference, arg, argReference});
  PageBuilder.buildPage(~wrapper, page);
};

let buildPageWithWrapper1 = (~arg, ~argReference, page) => {
  let wrapper =
    PageBuilder.Wrapper1({
      wrapper: ExampleWrapper1.wrapper,
      wrapperReference: ExampleWrapper1.wrapperReference,
      arg,
      argReference,
    });

  PageBuilder.buildPage(~wrapper, page);
};

let addPages = () => {
  buildPageWithWrapper1(pageIndex, ~arg="123", ~argReference={js|"123"|js});
  buildPageWithWrapper1(page1, ~arg="qwe", ~argReference={js|"qwe"|js});
};

let () = addPages();

let start = () => PageBuilder.start(~webpackOutputDir);

let build = () => PageBuilder.build(~webpackOutputDir);
