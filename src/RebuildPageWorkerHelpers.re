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
    : RebuildPageWorker.workerOutput =>
  // This is the place where we have to manually annotate output type of runWorker call
  WorkerThreads.runWorker(
    ~workerModulePath=Path.join2(dirname, "RebuildPageWorker.bs.js"),
    ~workerData,
    ~onExit,
  );

let buildPageWithWorker =
    (
      ~outputDir: string,
      ~logger: Log.logger,
      ~globalEnvValues: array((string, string)),
      page: PageBuilder.page,
    ) => {
  let rebuildPages = mapPageToPageForRebuild(~page, ~outputDir);

  let workerData: RebuildPageWorkerT.workerData = {
    page: rebuildPages,
    logLevel: logger.logLevel,
    globalEnvValues,
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
      ~globalEnvValues: array((string, string)),
      ~buildWorkersCount: option(int),
      ~exitOnPageBuildError: bool,
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

  let results =
    pages
    ->Array.splitIntoChunks(~chunkSize=buildWorkersCount)
    ->Js.Array2.map((pages, ()) =>
        pages
        ->Js.Array2.map(page =>
            buildPageWithWorker(~outputDir, ~logger, ~globalEnvValues, page)
          )
        ->Js.Promise.all
      )
    ->Promise.seqRun
    ->Promise.map(results => {
        logger.info(() => Js.Console.timeEnd(durationLabel));
        Array.flat1(results);
      });

  results->Promise.map(webpackPages =>
    webpackPages->Belt.Array.keepMap(result => {
      switch (result) {
      | Ok(webpackPage) => Some(webpackPage)
      | Error(path) =>
        Js.Console.error2(
          "[Commands.buildPagesWithWorkers] One of the pages failed to build:",
          PageBuilderT.PagePath.toString(path),
        );
        if (exitOnPageBuildError) {
          Process.exit(1);
        } else {
          None;
        };
      }
    })
  );
};
