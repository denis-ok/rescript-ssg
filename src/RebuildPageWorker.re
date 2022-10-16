let showPages = (pages: array(RebuildPageWorkerT.rebuildPage)) => {
  pages->Js.Array2.map(page => {
    Log.makeMinimalPrintablePageObj(
      ~pagePath=page.path,
      ~pageModulePath=page.modulePath,
    )
  });
};

[@val] external import_: string => Js.Promise.t('a) = "import";

let workerData: RebuildPageWorkerT.workerData = WorkingThreads.workerData;

let parentPort = WorkingThreads.parentPort;

let pages = workerData.pages;

let logSetting = workerData.logSetting;

let logger = Log.makeLogger(logSetting);

Js.log2("[Worker] Pages to rebuild:\n", pages->showPages);

pages
->Js.Array2.map(page => {
    let modulePath = page.modulePath;
    let outputDir = page.outputDir;

    Js.log2("[Worker] Trying to import module: ", modulePath);
    let importedModule = import_(modulePath);

    let importedWrapperModule =
      switch (page.pageWrapper) {
      | None => Js.Promise.resolve(None)
      | Some({modulePath, _}) =>
        Js.log2("[Worker] Trying to import wrapper module: ", modulePath);
        import_(modulePath)->Promise.map(module_ => Some(module_));
      };

    let modules = Js.Promise.all2((importedModule, importedWrapperModule));
    modules->Promise.map(((module_, wrapperModule)) => {
      let newPage: PageBuilder.page = {
        pageWrapper: {
          switch (page.pageWrapper, wrapperModule) {
          | (Some({component, modulePath}), Some(wrapperModule)) =>
            switch (component) {
            | WrapperWithChildren =>
              Some({
                component:
                  WrapperWithChildren(
                    children =>
                      React.createElement(
                        wrapperModule##make,
                        {"children": children},
                      ),
                  ),
                modulePath,
              })
            | RebuildPageWorkerT.WrapperWithDataAndChildren({data}) =>
              Some({
                component:
                  WrapperWithDataAndChildren({
                    component: (data, children) =>
                      React.createElement(
                        wrapperModule##make,
                        {"data": data, "children": children}->Obj.magic,
                      ),
                    data,
                  }),
                modulePath,
              })
            }
          | _ => None
          };
        },
        component: {
          switch (page.component) {
          | RebuildPageWorkerT.ComponentWithoutData =>
            ComponentWithoutData(
              React.createElement(module_##make, Js.Obj.empty()),
            )
          | RebuildPageWorkerT.ComponentWithData({data}) =>
            ComponentWithData({
              component: _propValue => {
                React.createElement(
                  module_##make,
                  {"data": data}->Obj.magic,
                );
              },
              data,
            })
          };
        },
        modulePath: module_##modulePath,
        headCssFilepaths: page.headCssFilepaths,
        path: page.path,
      };

      PageBuilder.buildPageHtmlAndReactApp(~outputDir, ~logger, newPage);
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
