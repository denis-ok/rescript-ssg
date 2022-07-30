type componentWithData('a) = {
  //
  data: 'a,
};

// This is almost the same type as in PageBuilder module but without component itself,
// because we can't pass functions to workers.

type component =
  | ComponentWithoutData
  | ComponentWithData(componentWithData('a)): component;

type rebuildPage = {
  component,
  modulePath: string,
  outputDir: string,
  path: PageBuilderT.PagePath.t,
};

type workerData = array(rebuildPage);
