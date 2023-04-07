let dirname = Utils.getDirname();

let mapPageToPageForRebuild =
    (~page: PageBuilder.page, ~outputDir): RebuildPageWorkerT.workerPage => {
  {
    pageWrapper: {
      switch (page.pageWrapper) {
      | None => None
      | Some({component: WrapperWithChildren(_), modulePath}) =>
        Some({component: WrapperWithChildren, modulePath})
      | Some({
          component: PageBuilder.WrapperWithDataAndChildren({data, _}),
          modulePath,
        }) =>
        Some({
          component: WrapperWithDataAndChildren({data: data}),
          modulePath,
        })
      };
    },
    component: {
      switch (page.component) {
      | ComponentWithoutData(_) => ComponentWithoutData
      | PageBuilder.ComponentWithData({data, _}) =>
        ComponentWithData({data: data})
      };
    },
    modulePath: page.modulePath,
    outputDir,
    headCssFilepaths: page.headCssFilepaths,
    path: page.path,
    globalValues: page.globalValues,
  };
};

let runRebuildPageWorker =
    (~onExit, ~workerData: RebuildPageWorkerT.workerData)
    : Js.Promise.t(array(Webpack.page)) =>
  WorkingThreads.runWorker(
    ~workerModulePath=Path.join2(dirname, "RebuildPageWorker.bs.js"),
    ~workerData,
    ~onExit,
  );

let rebuildPagesWithWorker =
    (
      ~outputDir: string,
      ~logger: Log.logger,
      ~globalValues: array((string, string)),
      pages: array(PageBuilder.page),
    ) => {
  let rebuildPages =
    pages->Js.Array2.map(page => mapPageToPageForRebuild(~page, ~outputDir));

  let workerData: RebuildPageWorkerT.workerData = {
    pages: rebuildPages,
    logLevel: logger.logLevel,
    globalValues,
  };

  runRebuildPageWorker(~workerData, ~onExit=exitCode => {
    logger.debug(() => Js.log2("[Worker] Exit code:", exitCode))
  });
};
