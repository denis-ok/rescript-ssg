// It's more reliable to have a constant for the project root directory and build paths relative to it
// instead of building paths relative to the directory of the current module.
// In the case of Melange, JS files are emitted to a different directory with a different nesting structure
// which can lead to issues. So better to use project root dir as the base.

[@val]
external projectRootDir': option(string) = "process.env.PROJECT_ROOT_DIR";

let projectRootDir =
  switch (projectRootDir') {
  | Some(dir) => dir
  | _ =>
    Js.Console.error("PROJECT_ROOT_DIR env var is missing");
    Process.exit(1);
  };

let outputDir = Path.join2(projectRootDir, "example/build");

let normalizeCssFilePath =
  Path.join2(projectRootDir, "example/src/css/normalize.css");

let globalEnvValues = [|
  ("process.env.ENV_VAR", Env.envVar),
  ("GLOBAL_VAR", "BAR"),
|];

let wrapperWithoutData: PageBuilder.pageWrapper = (
  {
    PageBuilder.component:
      WrapperWithChildren(
        children => <WrapperWithoutData> children </WrapperWithoutData>,
      ),
    modulePath: WrapperWithoutData.modulePath,
  }: PageBuilder.pageWrapper
);

let wrapperWithData: PageBuilder.pageWrapper = (
  {
    component:
      WrapperWithDataAndChildren({
        component: (data, children) =>
          <WrapperWithData data> children </WrapperWithData>,
        data: "LALA \"escaped quotes\"",
      }),
    modulePath: WrapperWithData.modulePath,
  }: PageBuilder.pageWrapper
);

let pageWithoutData: PageBuilder.page = (
  {
    hydrationMode: FullHydration,
    pageWrapper: None,
    component: ComponentWithoutData(<PageWithoutData />),
    modulePath: PageWithoutData.modulePath,
    headCssFilepaths: [|normalizeCssFilePath|],
    path: Path([|Page.toSlug(PageWithoutData)|]),
    globalValues:
      Some([|
        ("PER_PAGE_GLOBAL_1", "ONE!"->Js.Json.string),
        ("PER_PAGE_GLOBAL_2", "TWO!"->Js.Json.string),
      |]),
    headScripts: [||],
    bodyScripts: [||],
  }: PageBuilder.page
);

let pageWithData: PageBuilder.page = (
  {
    hydrationMode: FullHydration,
    pageWrapper: None,
    component:
      ComponentWithData({
        component: data => <PageWithData data />,
        data:
          Some({
            string: "foo \"bar\" baz",
            int: 1,
            float: 1.23,
            variant: One,
            polyVariant: `hello,
            option: Some("lalala"),
            bool: true,
          }),
      }),
    modulePath: PageWithData.modulePath,
    headCssFilepaths: [|normalizeCssFilePath|],
    path: Path([|Page.toSlug(PageWithData)|]),
    globalValues: None,
    headScripts: [||],
    bodyScripts: [||],
  }: PageBuilder.page
);

let pageWithoutDataAndWrapperWithoutData = {
  ...pageWithoutData,
  pageWrapper: Some(wrapperWithoutData),
  path: Path([|Page.toSlug(PageWithoutDataAndWrapperWithoutData)|]),
};

let pageWithoutDataAndWrapperWithData = {
  ...pageWithoutData,
  pageWrapper: Some(wrapperWithData),
  path: Path([|Page.toSlug(PageWithoutDataAndWrapperWithData)|]),
};

let pageWithDataAndWrapperWithoutData = {
  ...pageWithData,
  pageWrapper: Some(wrapperWithoutData),
  path: Path([|Page.toSlug(PageWithDataAndWrapperWithoutData)|]),
};

let pageWithDataAndWrapperWithData = {
  ...pageWithData,
  pageWrapper: Some(wrapperWithData),
  path: Path([|Page.toSlug(PageWithDataAndWrapperWithData)|]),
};

let pageWithoutDataDynamicPath: PageBuilder.page = (
  {
    ...pageWithoutData,
    component: ComponentWithoutData(<PageDynamic />),
    modulePath: PageDynamic.modulePath,
    path: Path([|Page.toSlug(PageWithoutData), PagePath.dynamicSegment|]),
  }: PageBuilder.page
);

let pageWithoutDataRegularPath: PageBuilder.page = (
  {
    ...pageWithoutData,
    component: ComponentWithoutData(<PageDynamic />),
    modulePath: PageDynamic.modulePath,
    // This page is needed to test esbuild watch mode.
    // We need to make sure that proxy server doesn't handle this path as a path with dynamic segment.
    // It makes sense to add a test for this case.
    path: Path([|Page.toSlug(PageWithoutData), "foo"|]),
  }: PageBuilder.page
);

let pageWithPartialHydration: PageBuilder.page = (
  {
    ...pageWithoutData,
    hydrationMode: PartialHydration,
    component: ComponentWithoutData(<PageWithPartialHydration />),
    modulePath: PageWithPartialHydration.modulePath,
    path: Path([|Page.toSlug(PageWithPartialHydration)|]),
  }: PageBuilder.page
);

let pageWithoutHydration: PageBuilder.page = (
  {
    ...pageWithoutData,
    hydrationMode: PartialHydration,
    component: ComponentWithoutData(<PageWithoutHydration />),
    modulePath: PageWithoutHydration.modulePath,
    path: Path([|Page.toSlug(PageWithoutHydration)|]),
  }: PageBuilder.page
);

let pages = [|
  {...pageWithoutData, path: Root},
  pageWithoutData,
  // pageWithoutDataAndWrapperWithoutData,
  // pageWithoutDataAndWrapperWithData,
  // pageWithData,
  // pageWithDataAndWrapperWithoutData,
  // pageWithDataAndWrapperWithData,
  // pageWithoutDataDynamicPath,
  // pageWithoutDataRegularPath,
  // pageWithPartialHydration,
  // pageWithoutHydration,
|];

let fakeExtralanguages = [|"es"|];

let localizedPages =
  Js.Array2.map(fakeExtralanguages, language =>
    Js.Array2.map(pages, page =>
      {
        ...page,
        path:
          switch (page.path) {
          | Root => Path([|language|])
          | Path(segments) =>
            Path(Js.Array2.concat([|language|], segments))
          },
      }
    )
  );

let pages = Js.Array2.concat([|pages|], localizedPages);
