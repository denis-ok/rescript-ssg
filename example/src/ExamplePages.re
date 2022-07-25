let currentDir = Utils.getDirname();

let pagesOutputDir = Path.join2(currentDir, "../build");

let webpackOutputDir = Path.join2(pagesOutputDir, "bundle");

// let makeWrapper = (~arg, ~argReference) =>
//   Some(
//     PageBuilder.Wrapper1({
//       wrapper: ExampleWrapper1.wrapper,
//       wrapperReference: ExampleWrapper1.wrapperReference,
//       arg,
//       argReference,
//     }),
//   );

let pageIndex: PageBuilder.page('a) = {
  component: ComponentWithoutProps(<ExampleIndex />),
  moduleName: ExampleIndex.moduleName,
  modulePath: ExampleIndex.modulePath,
  path: ".",
};

let page1: PageBuilder.page('a) = {
  component: ComponentWithoutProps(<ExamplePage1 />),
  moduleName: ExamplePage1.moduleName,
  modulePath: ExamplePage1.modulePath,
  path: "page1",
};

// TODO Fix localized dynamic path (issue with "@@@")

// let pageDynamic = {
//   PageBuilder.wrapper: None,
//   component: <ExamplePageDynamic />,
//   moduleName: ExamplePageDynamic.moduleName,
//   modulePath: ExamplePageDynamic.modulePath,
//   path: "page1/@@@",
// };

let languages = ["en", "ru"];

let pages = [pageIndex, page1];

let localizedPages =
  languages
  ->Belt.List.map(language => {
      pages->Belt.List.map(page =>
        {...page, path: Path.join2(language, page.path)}
      )
    })
  ->Belt.List.flatten;

let pages = Belt.List.concat(pages, localizedPages);

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
