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
  headCssFiles: array(string),
  path: PageBuilderT.PagePath.t,
};

type workerData = array(rebuildPage);
