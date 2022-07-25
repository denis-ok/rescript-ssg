type rebuildPage = {
  modulePath: string,
  outputDir: string,
  path: string,
};

type workerData = array(rebuildPage);
