[@val] external import_: string => Js.Promise.t('a) = "import";

let showPages = (pages: array(RebuildPageWorkerT.workerPage)) => {
  pages->Js.Array2.map(page => {
    Log.makeMinimalPrintablePageObj(
      ~pagePath=page.path,
      ~pageModulePath=page.modulePath,
    )
  });
};

let workerData: RebuildPageWorkerT.workerData = WorkingThreads.workerData;

let () = GlobalValues.unsafeAdd(workerData.globalValues);

let parentPort = WorkingThreads.parentPort;

let pages = workerData.pages;

let logLevel = workerData.logLevel;

let logger = Log.makeLogger(logLevel);

let successText = "[Worker] Pages build success. Duration";

Js.Console.timeStart(successText);

logger.info(() =>
  Js.log2(
    "[Worker] Pages to build:\n",
    pages->Js.Array2.map(page => PageBuilderT.PagePath.toString(page.path)),
  )
);

logger.debug(() => Js.log2("[Worker] Pages to build:\n", pages->showPages));

pages
->Js.Array2.map(page => {
    let () =
      page.globalValues
      ->Belt.Option.map(globalValues => GlobalValues.unsafeAdd(globalValues))
      ->ignore;

    let modulePath = page.modulePath;
    let outputDir = page.outputDir;

    logger.debug(() =>
      Js.log2("[Worker] Trying to import module: ", modulePath)
    );

    let importedModule = import_(modulePath);

    let importedWrapperModule =
      switch (page.pageWrapper) {
      | None => Js.Promise.resolve(None)
      | Some({modulePath, _}) =>
        logger.debug(() =>
          Js.log2("[Worker] Trying to import wrapper module: ", modulePath)
        );
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
        globalValues: page.globalValues,
      };

      PageBuilder.buildPageHtmlAndReactApp(~outputDir, ~logger, newPage);
    });
  })
->Js.Promise.all
->Promise.map((webpackPages: array(Webpack.page)) => {
    logger.info(() => Js.Console.timeEnd(successText));
    parentPort->WorkingThreads.postMessage(webpackPages);
  })
->Promise.catch(error => {
    logger.info(() =>
      Js.Console.error2(
        "[Worker] [Warning] Caught error, please check: ",
        error,
      )
    );
    Js.Promise.resolve();
  })
->ignore;
