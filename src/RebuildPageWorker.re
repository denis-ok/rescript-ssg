[@val] external import_: string => Js.Promise.t('a) = "import";

let showPage = (page: RebuildPageWorkerT.workerPage) => {
  Log.makeMinimalPrintablePageObj(
    ~pagePath=page.path,
    ~pageModulePath=page.modulePath,
  );
};

let workerData: RebuildPageWorkerT.workerData = WorkingThreads.workerData;

let () = GlobalValues.unsafeAdd(workerData.globalEnvValues);

let parentPort = WorkingThreads.parentPort;

let page = workerData.page;

let logLevel = workerData.logLevel;

let logger = Log.makeLogger(logLevel);

let successText = "[Worker] Pages build success. Duration";

Js.Console.timeStart(successText);

logger.info(() =>
  Js.log2(
    "[Worker] Page to build:\n",
    PageBuilderT.PagePath.toString(page.path),
  )
);

logger.debug(() => Js.log2("[Worker] Page to build:\n", page->showPage));

let () =
  page.globalValues
  ->Belt.Option.map(globalValues => GlobalValues.unsafeAddJson(globalValues))
  ->ignore;

let modulePath = page.modulePath;
let outputDir = page.outputDir;

logger.debug(() => Js.log2("[Worker] Trying to import module: ", modulePath));

let pageModule = import_(modulePath);

let pageWrapperModule =
  switch (page.pageWrapper) {
  | None => Js.Promise.resolve(None)
  | Some({modulePath, _}) =>
    logger.debug(() =>
      Js.log2("[Worker] Trying to import wrapper module: ", modulePath)
    );
    import_(modulePath)->Promise.map(module_ => Some(module_));
  };

let importedModules = Js.Promise.all2((pageModule, pageWrapperModule));

type workerOutput =
  Js.Promise.t(
    Belt.Result.t(Webpack.page, RescriptSsg.PageBuilderT.PagePath.t),
  );

let workerOutput: workerOutput =
  importedModules
  ->Promise.map(((module_, wrapperModule)) => {
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
        globalValues: page.globalValues,
      };

      PageBuilder.buildPageHtmlAndReactApp(~outputDir, ~logger, newPage);
    })
  ->Promise.map((webpackPage: Webpack.page) => {
      logger.info(() => Js.Console.timeEnd(successText));
      let result = Belt.Result.Ok(webpackPage);
      parentPort->WorkingThreads.postMessage(result);
      result;
    })
  ->Promise.catch(error => {
      // We don't want to immediatelly stop node process/watcher when something happened in worker.
      // We just log the error and the caller will decide what to do.
      logger.info(() =>
        Js.Console.error2(
          "[Worker] [Warning] Caught error, please check: ",
          error,
        )
      );
      let result = Belt.Result.Error(page.path);
      parentPort->WorkingThreads.postMessage(result);
      Js.Promise.resolve(result);
    });
