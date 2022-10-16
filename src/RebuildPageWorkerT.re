// Here we have almost the same type as in PageBuilder module but constructor don't accept functions,
// because we can't pass functions to workers.

type componentWithData('a) = {data: 'a};

type wrapperComponent =
  | WrapperWithChildren
  | WrapperWithDataAndChildren(componentWithData('a)): wrapperComponent;

type pageWrapper = {
  component: wrapperComponent,
  modulePath: string,
};

type component =
  | ComponentWithoutData
  | ComponentWithData(componentWithData('a)): component;

type rebuildPage = {
  pageWrapper: option(pageWrapper),
  component,
  modulePath: string,
  outputDir: string,
  headCssFilepaths: array(string),
  path: PageBuilderT.PagePath.t,
};

type workerData = array(rebuildPage);

let showPage = (page: rebuildPage) => {
  PageBuilderT.PagePath.toString(page.path);
};

let showPages = (pages: array(rebuildPage)) => {
  pages->Js.Array2.map(page => page->showPage);
};
