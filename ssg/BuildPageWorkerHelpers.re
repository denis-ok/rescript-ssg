let dirname = Utils.getDirname();

let mapPageToPageForRebuild =
    (~page: PageBuilder.page): BuildPageWorkerT.workerPage => {
  {
    hydrationMode: page.hydrationMode,
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
    headCssFilepaths: page.headCssFilepaths,
    path: page.path,
    globalValues: page.globalValues,
    headScripts: page.headScripts,
    bodyScripts: page.bodyScripts,
  };
};

let runBuildPageWorker =
    (~onExit, ~workerData: BuildPageWorkerT.workerData)
    : BuildPageWorker.workerOutput =>
  // This is the place where we have to manually annotate output type of runWorker call
  WorkerThreads.runWorker(
    ~workerModulePath=Path.join2(dirname, "BuildPageWorker.bs.js"),
    ~workerData,
    ~onExit,
  );

let buildPagesWithWorker =
    (
      ~outputDir: string,
      ~melangeOutputDir: option(string),
      ~logger: Log.logger,
      ~globalEnvValues: array((string, string)),
      ~generatedFilesSuffix: string,
      pages: array(PageBuilder.page),
    ) => {
  let rebuildPages =
    pages->Js.Array2.map(page => mapPageToPageForRebuild(~page));

  let workerData: BuildPageWorkerT.workerData = {
    outputDir,
    melangeOutputDir,
    pages: rebuildPages,
    logLevel: logger.logLevel,
    globalEnvValues,
    generatedFilesSuffix,
  };

  runBuildPageWorker(~workerData, ~onExit=exitCode => {
    logger.debug(() => Js.log2("[Worker] Exit code:", exitCode))
  });
};

let defaultWorkersCount = 16;

let buildPagesWithWorkers =
    (
      ~pages: array(array(PageBuilder.page)),
      ~outputDir: string,
      ~melangeOutputDir: option(string),
      ~logger: Log.logger,
      ~globalEnvValues: array((string, string)),
      ~buildWorkersCount: option(int),
      ~exitOnPageBuildError: bool,
      ~generatedFilesSuffix: string,
    )
    : Js.Promise.t(array(RenderedPage.t)) => {
  let buildWorkersCount =
    switch (buildWorkersCount) {
    | None =>
      switch (NodeOs.availableParallelism) {
      | None => defaultWorkersCount
      | Some(f) => min(f(), defaultWorkersCount)
      }
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

  // let pages = pages->Array.splitIntoChunks(~chunkSize=buildWorkersCount);

  let pagesManualChunks = pages;

  let results =
    pagesManualChunks
    ->Js.Array2.map((pagesChunk, ()) =>
        buildPagesWithWorker(
          ~outputDir,
          ~melangeOutputDir,
          ~logger,
          ~globalEnvValues,
          ~generatedFilesSuffix,
          pagesChunk,
        )
      )
    ->Promise.seqRun
    ->Promise.map(results => {
        logger.info(() => Js.Console.timeEnd(durationLabel));
        Array.flat1(results);
      });

  results->Promise.map(renderedPages =>
    renderedPages->Belt.Array.keepMap(result => {
      switch (result) {
      | Ok(renderedPage) => Some(renderedPage)
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
