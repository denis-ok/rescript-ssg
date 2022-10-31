let currentDir = Utils.getDirname()

let outputDir = Path.join2(currentDir, "../build")

let normalizeCssFilePath = Path.join2(currentDir, "/css/normalize.css")

let pageIndex: PageBuilder.page = {
  pageWrapper: Some({
    component: WrapperWithChildren(children => <Wrapper> children </Wrapper>),
    modulePath: Wrapper.modulePath,
  }),
  component: ComponentWithoutData(<Index />),
  modulePath: Index.modulePath,
  headCssFilepaths: [normalizeCssFilePath],
  path: Root,
}

let page1: PageBuilder.page = {
  pageWrapper: Some({
    component: WrapperWithDataAndChildren({
      component: (data, children) => <WrapperWithData data> children </WrapperWithData>,
      data: "LALA",
    }),
    modulePath: WrapperWithData.modulePath,
  }),
  component: ComponentWithData({
    component: data => <Page1 data />,
    data: Some({
      string: "lala",
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
}

let page11: PageBuilder.page = {
  pageWrapper: None,
  component: ComponentWithData({
    component: data => <Page1 data />,
    data: Some({
      string: "lala",
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
}

let page2: PageBuilder.page = {
  pageWrapper: None,
  component: ComponentWithData({component: data => <Page2 data />, data: true}),
  modulePath: Page2.modulePath,
  headCssFilepaths: [],
  path: Path(["page2"]),
}

let page1Dynamic: PageBuilder.page = {
  pageWrapper: None,
  component: ComponentWithoutData(<PageDynamic />),
  modulePath: PageDynamic.modulePath,
  headCssFilepaths: [],
  path: Path(["page1", "_id"]),
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
      | Path(parts) => Path(Js.Array2.concat([language], parts))
      },
    })
  )
  ->Array.flat1

let pages = Js.Array2.concat(pages, localizedPages)
