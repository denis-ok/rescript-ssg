type prop('a) = {
  name: string,
  value: 'a,
};

type componentWithOneProp('a) = {
  //
  prop: prop('a),
};

// This is almost the same type as in PageBuilder module but without component itself,
// because we can't pass functions to workers.

type component =
  | ComponentWithoutProps
  | ComponentWithOneProp(componentWithOneProp('a)): component;

type rebuildPage = {
  component,
  modulePath: string,
  outputDir: string,
  path: PageBuilderT.PagePath.t,
};

type workerData = array(rebuildPage);
