// Here we have almost the same type as in PageBuilder module but constructord don't accept functions,
// because we can't pass functions to workers.

type componentWithData('a) = {data: 'a};

type wrapperComponent =
  | ComponentWithChildrenOnly;

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
  path: PageBuilderT.PagePath.t,
};

type workerData = array(rebuildPage);
