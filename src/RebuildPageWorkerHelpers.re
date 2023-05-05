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

let buildPagesWithWorkers =
    (
      ~pages: array(PageBuilder.page),
      ~outputDir: string,
      ~logger: Log.logger,
      ~globalValues: array((string, string)),
      ~buildWorkersCount: option(int),
    ) => {
  let buildWorkersCount =
    switch (buildWorkersCount) {
    | None =>
      // Using 16 as some reasonable limit for workers count
      min(NodeOs.availableParallelism(), 16)
    | Some(buildWorkersCount) => buildWorkersCount
    };

  logger.info(() =>
    Js.log3(
      "[Commands.buildPagesWithWorkers] Building pages with ",
      buildWorkersCount,
      " workers...",
    )
  );

  let durationLabel = "[Commands.buildPagesWithWorkers] Build finished. Duration";
  Js.Console.timeStart(durationLabel);

  pages
  ->Array.splitIntoChunks(~chunkSize=buildWorkersCount)
  ->Js.Array2.map((pages, ()) =>
      pages
      ->Js.Array2.map(page =>
          rebuildPagesWithWorker(
            ~outputDir,
            ~logger,
            ~globalValues,
            [|page|],
          )
        )
      ->Js.Promise.all
      ->Promise.map(result => Array.flat1(result))
    )
  ->Promise.seqRun
  ->Promise.map(result => {
      logger.info(() => Js.Console.timeEnd(durationLabel));
      Array.flat1(result);
    });
};
