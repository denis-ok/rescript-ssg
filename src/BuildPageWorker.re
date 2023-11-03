module DomParser = {
  // Quick workaround to fix react-intl error: Cannot format XML message without DOMParser.
  // https://github.com/vercel/next.js/issues/10533#issuecomment-587477942
  // This should be fixed more elegantly and removed from here.
  type domParser;

  [@module "@xmldom/xmldom"] external domParser: domParser = "DOMParser";

  [@val] external globalThis: Js.Dict.t(domParser) = "globalThis";

  let () = globalThis->Js.Dict.set("DOMParser", domParser);
};

[@bs.val] external import_: string => Js.Promise.t('a) = "import";

let showPage = (page: BuildPageWorkerT.workerPage) => {
  Log.makeMinimalPrintablePageObj(
    ~pagePath=page.path,
    ~pageModulePath=page.modulePath,
  );
};

let workerData: BuildPageWorkerT.workerData = WorkerThreads.workerData;

let parentPort = WorkerThreads.parentPort;

let pages = workerData.pages;

let pagesCount: string = workerData.pages->Js.Array2.length->Belt.Int.toString;

let logger = Log.makeLogger(workerData.logLevel);

logger.info(() => Js.log({j|[Worker] Building $(pagesCount) pages...|j}));

type workerOutput =
  Promise.t(array(Belt.Result.t(RenderedPage.t, PagePath.t)));

let workerOutput: workerOutput =
  pages
  ->Js.Array2.map(page => {
      let moduleName: string =
        Utils.getModuleNameFromModulePath(page.modulePath);

      let pagePath: string = page.path->PagePath.toString;

      let pageInfo: string = {j|[Page module: $(moduleName), page path: $(pagePath)]|j};

      let successText = {j|[Worker] $(pageInfo) Build success. Duration|j};

      Js.Console.timeStart(successText);

      logger.info(() => {Js.log({j|[Worker] $(pageInfo) Building...|j})});

      logger.debug(() =>
        Js.log2("[Worker] Page to build:\n", page->showPage)
      );

      let () = GlobalValues.unsafeAdd(workerData.globalEnvValues);

      let () =
        page.globalValues
        ->Belt.Option.forEach(globalValues =>
            GlobalValues.unsafeAddJson(globalValues)
          );

      logger.debug(() =>
        Js.log2("[Worker] Trying to import page module: ", page.modulePath)
      );

      let pageModule = import_(page.modulePath);

      let pageWrapperModule =
        switch (page.pageWrapper) {
        | None => Promise.resolve(None)
        | Some({modulePath, _}) =>
          logger.debug(() =>
            Js.log2(
              "[Worker] Trying to import page wrapper module: ",
              modulePath,
            )
          );
          import_(modulePath)->Promise.map(module_ => Some(module_));
        };

      let importedModules = Promise.all2((pageModule, pageWrapperModule));

      importedModules
      ->Promise.flatMap(((module_, wrapperModule)) => {
          let newPage: PageBuilder.page = {
            hydrationMode: page.hydrationMode,
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
                | BuildPageWorkerT.WrapperWithDataAndChildren({data}) =>
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
              | BuildPageWorkerT.ComponentWithoutData =>
                ComponentWithoutData(
                  React.createElement(module_##make, Js.Obj.empty()),
                )
              | BuildPageWorkerT.ComponentWithData({data}) =>
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
            headScripts: page.headScripts,
            bodyScripts: page.bodyScripts,
          };

          PageBuilder.buildPageHtmlAndReactApp(
            ~pageAppArtifactsType=workerData.pageAppArtifactsType,
            ~outputDir=workerData.outputDir,
            ~melangeOutputDir=workerData.melangeOutputDir,
            ~logger,
            ~pageAppArtifactsSuffix=workerData.pageAppArtifactsSuffix,
            newPage,
          );
        })
      ->Promise.map(result => {
          switch (result) {
          | Ok((renderedPage: RenderedPage.t)) =>
            logger.info(() => Js.Console.timeEnd(successText));
            Belt.Result.Ok(renderedPage);
          | Error((errors: array((string, Js.Promise.error)))) =>
            logger.info(() => {
              Js.Console.error2(
                {j|[Worker] $(pageInfo) Build page errors:|j},
                errors,
              )
            });
            let result = Belt.Result.Error(page.path);
            result;
          }
        })
      ->Promise.catch(error => {
          // We don't want to immediately stop node process/watcher when something happened in worker.
          // We just log the error and the caller will decide what to do.
          logger.info(() => {
            Js.Console.error2(
              {j|[Worker] $(pageInfo) Unexpected promise rejection, please check:|j},
              error,
            )
          });
          let result = Belt.Result.Error(page.path);
          Promise.resolve(result);
        });
    })
  ->Promise.all
  ->Promise.map(pages => {
      parentPort->WorkerThreads.postMessage(pages);
      pages;
    });
