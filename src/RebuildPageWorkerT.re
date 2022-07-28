type rebuildPage = {
  modulePath: string,
  outputDir: string,
  path: PageBuilderT.PagePath.t,
};

type workerData = array(rebuildPage);
