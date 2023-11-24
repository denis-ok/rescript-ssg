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
      ~pageAppArtifactsType: PageBuilder.pageAppArtifactsType,
      ~outputDir: string,
      ~melangeOutputDir: option(string),
      ~logger: Log.logger,
      ~globalEnvValues: array((string, string)),
      ~pageAppArtifactsSuffix: string,
      pages: array(PageBuilder.page),
    ) => {
  let rebuildPages =
    pages->Js.Array2.map(page => mapPageToPageForRebuild(~page));

  let workerData: BuildPageWorkerT.workerData = {
    pageAppArtifactsType,
    outputDir,
    melangeOutputDir,
    pages: rebuildPages,
    logLevel: logger.logLevel,
    globalEnvValues,
    pageAppArtifactsSuffix,
  };

  runBuildPageWorker(~workerData, ~onExit=exitCode => {
    logger.debug(() => Js.log2("[Worker] Exit code:", exitCode))
  });
};

let defaultWorkersCount = 8;

let buildPagesWithWorkers =
    (
      ~pageAppArtifactsType: PageBuilder.pageAppArtifactsType,
      ~pages: array(array(PageBuilder.page)),
      ~outputDir: string,
      ~melangeOutputDir: option(string),
      ~logger: Log.logger,
      ~globalEnvValues: array((string, string)),
      ~buildWorkersCount: option(int),
      ~exitOnPageBuildError: bool,
      ~pageAppArtifactsSuffix: string,
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

  let minPagesCountForSplitting = 500;

  // We expect that user passed already chunked array of pages: array(array(pages))
  // where each array(pages) is a chunk for one worker thread.
  // The most common case is when user wants to build localized website and there are two chunks for two locales.
  // But we can have a situation when one locale contains too many pages and it's a good idea to split
  // user defined chunk into smaller chunks to spawn more workers and parallelize rendering.
  // For example user has 2 locales and 8 cores and and to spawn 8 workers we must do an extra chunk splitting.
  let pages =
    pages
    ->Js.Array2.map(pagesChunk => {
        let pagesInChunk = pagesChunk->Js.Array2.length;
        switch (pagesInChunk >= minPagesCountForSplitting) {
        | false => [|pagesChunk|]
        | true =>
          let chunksCount = pagesInChunk / minPagesCountForSplitting + 1;
          let chunkSize =
            pagesInChunk / chunksCount + pagesInChunk mod chunksCount;
          pagesChunk->Array.splitIntoChunks(~chunkSize);
        };
      })
    ->Array.flat1;

  logger.info(() =>
    Js.log3(
      "[Commands.buildPagesWithWorkers] Building pages with ",
      buildWorkersCount,
      " workers...",
    )
  );

  let startTime = Performance.now();

  let pagesChunkedForWorkers =
    pages->Array.splitIntoChunks(~chunkSize=buildWorkersCount);

  let results =
    pagesChunkedForWorkers
    ->Js.Array2.map((pagesChunk: array(array(PageBuilder.page))) => {
        let buildChunksWithWorkers = () =>
          pagesChunk
          ->Js.Array2.map(chunk =>
              buildPagesWithWorker(
                ~pageAppArtifactsType,
                ~outputDir,
                ~melangeOutputDir,
                ~logger,
                ~globalEnvValues,
                ~pageAppArtifactsSuffix,
                chunk,
              )
            )
          ->Promise.all;

        buildChunksWithWorkers;
      })
    ->Promise.seqRun
    ->Promise.map(results => Array.flat2(results))
    ->Promise.map(results => {
        Js.log2(
          "[Commands.buildPagesWithWorkers] Build finished. Duration",
          Performance.durationSinceStartTime(~startTime),
        );
        results;
      });

  results->Promise.map(renderedPages =>
    renderedPages->Belt.Array.keepMap(result => {
      switch (result) {
      | Ok(renderedPage) => Some(renderedPage)
      | Error(path) =>
        Js.Console.error2(
          "[Commands.buildPagesWithWorkers] One of the pages failed to build:",
          PagePath.toString(path),
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
