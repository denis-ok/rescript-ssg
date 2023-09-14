let currentDir = Utils.getDirname();

let outputDir = Path.join2(currentDir, "../build");

let normalizeCssFilePath = Path.join2(currentDir, "/css/normalize.css");

let globalEnvValues = [|
  ("process.env.ENV_VAR", Env.envVar),
  ("GLOBAL_VAR", "BAR"),
|];

let wrapperWithoutData: RescriptSsg.PageBuilder.pageWrapper = (
  {
    PageBuilder.component:
      WrapperWithChildren(
        children => <WrapperWithoutData> children </WrapperWithoutData>,
      ),
    modulePath: WrapperWithoutData.modulePath,
  }: RescriptSsg.PageBuilder.pageWrapper
);

let wrapperWithData: RescriptSsg.PageBuilder.pageWrapper = (
  {
    component:
      WrapperWithDataAndChildren({
        component: (data, children) =>
          <WrapperWithData data> children </WrapperWithData>,
        data: "LALA \"escaped quotes\"",
      }),
    modulePath: WrapperWithData.modulePath,
  }: RescriptSsg.PageBuilder.pageWrapper
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
    path: Path([|Page.toSlug(PageWithoutData), "dynamic__id"|]),
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
  pageWithoutDataAndWrapperWithoutData,
  pageWithoutDataAndWrapperWithData,
  pageWithData,
  pageWithDataAndWrapperWithoutData,
  pageWithDataAndWrapperWithData,
  pageWithoutDataDynamicPath,
  pageWithPartialHydration,
  pageWithoutHydration,
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