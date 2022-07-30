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

        let newPage: PageBuilder.page = {
          component: {
            switch (page.component) {
            | RebuildPageWorkerT.ComponentWithoutProps =>
              ComponentWithoutProps(
                React.createElement(module_##make, Js.Obj.empty()),
              )
            | RebuildPageWorkerT.ComponentWithOneProp({prop}) =>
              ComponentWithOneProp({
                component: _propValue => {
                  // TODO FIX ME. Use predefined prop name
                  React.createElement(
                    module_##make,
                    Js.Obj.empty(),
                  );
                },
                prop: {
                  name: prop.name,
                  value: prop.value,
                },
              })
            };
          },
          moduleName: module_##moduleName,
          modulePath: module_##modulePath,
          path: page.path,
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
