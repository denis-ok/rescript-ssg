let currentDir = Utils.getDirname();

let pagesOutputDir = Path.join2(currentDir, "../build");

let webpackOutputDir = Path.join2(pagesOutputDir, "bundle");

let pageIndex: PageBuilder.page = {
  component: ComponentWithoutProps(<ExampleIndex />),
  moduleName: ExampleIndex.moduleName,
  modulePath: ExampleIndex.modulePath,
  path: Root,
};

let page1: PageBuilder.page = {
  component:
    ComponentWithOneProp({
      component: pageContext => <ExamplePage1 pageContext />,
      prop: {
        name: "pageContext",
        value:
          Some({
            string: "lala",
            int: 1,
            float: 1.23,
            variant: One,
            polyVariant: `hello,
            option: Some("lalala"),
            bool: true,
          }),
      },
    }),
  moduleName: ExamplePage1.moduleName,
  modulePath: ExamplePage1.modulePath,
  path: Path([|"page1"|]),
};

let page11: PageBuilder.page = {
  component:
    ComponentWithOneProp({
      component: pageContext => <ExamplePage1 pageContext />,
      prop: {
        name: "pageContext",
        value:
          Some({
            string: "lala",
            int: 1,
            float: 1.23,
            variant: One,
            polyVariant: `hello,
            option: Some("lalala"),
            bool: true,
          }),
      },
    }),
  moduleName: ExamplePage1.moduleName,
  modulePath: ExamplePage1.modulePath,
  path: Path([|"page11"|]),
};

let page2: PageBuilder.page = {
  component:
    ComponentWithOneProp({
      component: boolProp => <ExamplePage2 boolProp />,
      prop: {
        name: "boolProp",
        value: true,
      },
    }),
  moduleName: ExamplePage2.moduleName,
  modulePath: ExamplePage2.modulePath,
  path: Path([|"page2"|]),
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

let pages = [pageIndex, page1, page2];

let localizedPages =
  languages
  ->Belt.List.map(language => {
      pages->Belt.List.map(page =>
        {
          ...page,
          path:
            switch (page.path) {
            | Root => Path([|language|])
            | Path(parts) => Path(Js.Array2.concat([|language|], parts))
            },
        }
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
