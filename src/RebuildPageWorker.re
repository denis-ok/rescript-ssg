type workerData = RebuildPageWorkerT.workerData;

let workerData: workerData = WorkingThreads.workerData;

let parentPort = WorkingThreads.parentPort;

let pages: workerData = workerData;

Js.log2("[Worker] Pages to rebuild: ", pages);

pages
->Js.Array2.map(page => {
    let modulePath = page.modulePath;
    let outputDir = page.outputDir;

    Js.log2("[Worker] Trying to do fresh import: ", modulePath);

    PageBuilder.import_(modulePath)
    ->Promise.map(module_ => {
        Js.log2("[Worker] Fresh import success: ", modulePath);

        let newPage = {
          wrapper: None,
          path: page.path,
          PageBuilder.component:
            React.createElement(module_##make, Js.Obj.empty()),
          moduleName: module_##moduleName,
          modulePath: module_##modulePath,
        };

        PageBuilder.buildPageHtmlAndReactApp(~outputDir, newPage);
      });
  })
->Js.Promise.all
->Promise.map((_: array(unit)) => {
    Js.log("[Worker] Pages rebuild success, job finished.");

    parentPort->WorkingThreads.postMessage({
      "originalData": workerData,
      "extraData": "Hello world",
    });
  })
->ignore;
