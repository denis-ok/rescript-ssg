type data = {
  modulePath: string,
  outputDir: string,
  path: string,
};

let workerData: data = WorkingThreads.workerData;

let parentPort = WorkingThreads.parentPort;

let {modulePath, outputDir, path} = workerData;

Js.log2("[rebuildPage] Trying to do fresh import: ", modulePath);

PageBuilder.import_(modulePath)
->Promise.map(module_ => {
    Js.log2("[rebuildPage] Fresh import success: ", modulePath);

    let newPage = {
      wrapper: None,
      path,
      PageBuilder.component:
        React.createElement(module_##make, Js.Obj.empty()),
      moduleName: module_##moduleName,
      modulePath: module_##modulePath,
    };

    PageBuilder.buildPageHtmlAndReactApp(~outputDir, newPage);

    parentPort->WorkingThreads.postMessage({
      "originalData": workerData,
      "extraData": "Hello world",
    });
  })
->ignore;
