let currentDir = Utils.getDirname()

let outputDir = Path.join2(currentDir, "../build")

let normalizeCssFilePath = Path.join2(currentDir, "/css/normalize.css")

let globalEnvValues = [
  //
  ("process.env.ENV_VAR", Env.envVar),
  ("GLOBAL_VAR", "BAR"),
]

let pageIndex: PageBuilder.page = {
  let jsonExample =
    [("string", "hello"->Js.Json.string), ("int", 2.->Js.Json.number)]
    ->Js.Dict.fromArray
    ->Js.Json.object_

  {
    pageWrapper: Some({
      component: WrapperWithChildren(children => <Wrapper> children </Wrapper>),
      modulePath: Wrapper.modulePath,
    }),
    component: ComponentWithoutData(<Index />),
    modulePath: Index.modulePath,
    headCssFilepaths: [normalizeCssFilePath],
    path: Root,
    globalValues: Some([
      ("PER_PAGE_GLOBAL_1", "INDEX"->Js.Json.string),
      ("PER_PAGE_GLOBAL_2", jsonExample),
    ]),
    headScripts: [],
    bodyScripts: [],
  }
}

let page1: PageBuilder.page = {
  pageWrapper: Some({
    component: WrapperWithDataAndChildren({
      component: (data, children) => <WrapperWithData data> children </WrapperWithData>,
      data: "LALA \"escape\"",
    }),
    modulePath: WrapperWithData.modulePath,
  }),
  component: ComponentWithData({
    component: data => <Page1 data />,
    data: Some({
      string: "foo \"bar\" baz",
      int: 1,
      float: 1.23,
      variant: One,
      polyVariant: #hello,
      option: Some("lalala"),
      bool: true,
    }),
  }),
  modulePath: Page1.modulePath,
  headCssFilepaths: [],
  path: Path(["page1"]),
  globalValues: Some([("PER_PAGE_GLOBAL_1", "PAGE1"->Js.Json.string)]),
  headScripts: [],
  bodyScripts: [],
}

let page11: PageBuilder.page = {
  pageWrapper: None,
  component: ComponentWithData({
    component: data => <Page1 data />,
    data: Some({
      string: "foo \"bar\" baz",
      int: 1,
      float: 1.23,
      variant: One,
      polyVariant: #hello,
      option: Some("lalala"),
      bool: true,
    }),
  }),
  modulePath: Page1.modulePath,
  headCssFilepaths: [],
  path: Path(["page11"]),
  globalValues: None,
  headScripts: [],
  bodyScripts: [],
}

let page2: PageBuilder.page = {
  pageWrapper: None,
  component: ComponentWithData({component: data => <Page2 data />, data: true}),
  modulePath: Page2.modulePath,
  headCssFilepaths: [],
  path: Path(["page2"]),
  globalValues: None,
  headScripts: [],
  bodyScripts: [],
}

let page1Dynamic: PageBuilder.page = {
  pageWrapper: None,
  component: ComponentWithoutData(<PageDynamic />),
  modulePath: PageDynamic.modulePath,
  headCssFilepaths: [],
  path: Path(["page1", "dynamic__id"]),
  globalValues: None,
  headScripts: [],
  bodyScripts: [],
}

let languages = ["en", "ru"]

let pages = [pageIndex, page1, page2, page1Dynamic]

let localizedPages =
  languages
  ->Js.Array2.map(language =>
    pages->Js.Array2.map(page => {
      ...page,
      path: switch page.path {
      | Root => Path([language])
      | Path(segments) => Path(Js.Array2.concat([language], segments))
      },
    })
  )
  ->Array.flat1

let pages = Js.Array2.concat(pages, localizedPages)
