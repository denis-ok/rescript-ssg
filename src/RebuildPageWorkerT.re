// Here we have almost the same type as in PageBuilder module but type constructor doesn't accept functions,
// because we can't pass functions to workers.
// https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Structured_clone_algorithm

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

type workerPage = {
  pageWrapper: option(pageWrapper),
  component,
  modulePath: string,
  outputDir: string,
  headCssFilepaths: array(string),
  path: PageBuilderT.PagePath.t,
  globalValues: option(array((string, Js.Json.t))),
  allowDarkMode: bool,
};

type workerData = {
  logLevel: Log.level,
  page: workerPage,
  globalEnvValues: array((string, string)),
};

let showPage = (page: workerPage) => {
  PageBuilderT.PagePath.toString(page.path);
};

let showPages = (pages: array(workerPage)) => {
  pages->Js.Array2.map(page => page->showPage);
};
