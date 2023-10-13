let uniqueStringArray = (array: array(string)) =>
  Set.fromArray(array)->Set.toArray;

let uniqueArray = (array: array('a), ~getId: 'a => string) => {
  let items = array->Js.Array2.map(v => (v->getId, v));
  items->Js.Dict.fromArray->Js.Dict.values;
};

let makeUniquePageArray = (pages: array(PageBuilder.page)) => {
  pages->uniqueArray(~getId=page => PagePath.toString(page.path));
};

let showPages = (pages: array(PageBuilder.page)) => {
  pages->Js.Array2.map(page => {
    Log.makeMinimalPrintablePageObj(
      ~pagePath=page.path,
      ~pageModulePath=page.modulePath,
    )
  });
};

let getModuleDependencies = (~pageModulePath) =>
  DependencyTree.makeList({
    filename: pageModulePath,
    // TODO Fix me. Is it really needed? Should it be func argument?
    directory: ".",
    filter: path => path->Js.String2.indexOf("node_modules") == (-1),
  });

// To make a watcher work properly we need to:
// 1. Watch for the changes in a root module (page module).
// 2. Watch for the changes in all dependencies of a root module (except node modules).
// 3. Watch for the changes in wrapper components and handle them as dependencies of a root module.
// 4. Watch for the changes in headCssFilepaths.

// After a module changes we should rebuild a page and refresh dependency dicts to remove the stale ones.

// The watcher logic looks like this:
// We detect change in some file.
// If the change is in the exact page module -> simply get pages from modulePathToPagesDict and rebuild them.
// If the change is in some dependency -> get root modules from a dependency -> get pages from root modules.
// If the change is in a head CSS file -> get pages that use this css file and rebuild them.

let startWatcher =
    (
      ~pageAppArtifact: PageBuilder.pageAppArtifact,
      ~outputDir: string,
      ~melangeOutputDir: option(string),
      ~logger: Log.logger,
      ~globalEnvValues: array((string, string)),
      ~buildWorkersCount: option(int)=?,
      pages: array(array(PageBuilder.page)),
    )
    : unit => {
  let pages = Array.flat1(pages);

  let durationLabel = "[Watcher] Watching for file changes... Startup duration";
  Js.Console.timeStart(durationLabel);

  let modulePathToPagesDict = Js.Dict.empty();
  let headCssFileToPagesDict = Js.Dict.empty();

  pages->Js.Array2.forEach(page => {
    // Multiple pages can use the same page module. The common case is localized pages.
    // We populate modulePathToPagesDict to get modulePath -> array(pages) dict here.
    switch (modulePathToPagesDict->Js.Dict.get(page.modulePath)) {
    | None => modulePathToPagesDict->Js.Dict.set(page.modulePath, [|page|])
    | Some(pages) => pages->Js.Array2.push(page)->ignore
    };

    page.headCssFilepaths
    ->Js.Array2.forEach(headCssFile => {
        switch (headCssFileToPagesDict->Js.Dict.get(headCssFile)) {
        | None => headCssFileToPagesDict->Js.Dict.set(headCssFile, [|page|])
        | Some(pages) =>
          pages->Js.Array2.push(page)->ignore;
          // We can try storing a tuple (array, set) and check set before pushing
          headCssFileToPagesDict->Js.Dict.set(
            headCssFile,
            pages->makeUniquePageArray,
          );
        }
      });
  });

  let pageModulePaths = modulePathToPagesDict->Js.Dict.keys;

  let pageModulesAndTheirDependencies =
    pageModulePaths->Js.Array2.map(pageModulePath => {
      // This should be changed to esbuild function.
      let dependencies = getModuleDependencies(~pageModulePath);
      (pageModulePath, dependencies);
    });

  // Dependency is a some import in page's main module (page.modulePath).
  // Multiple pages can depend on the same dependency.
  // The dict below maps dependency path to the array of page module paths.
  let dependencyToPageModulesDict = Js.Dict.empty();

  let updateDependencyToPageModulesDict = (~dependency, ~pageModulePath) => {
    switch (dependencyToPageModulesDict->Js.Dict.get(dependency)) {
    | None =>
      dependencyToPageModulesDict->Js.Dict.set(
        dependency,
        [|pageModulePath|],
      )
    | Some(pageModulePaths) =>
      // We can try storing a tuple (array, set) and check set before pushing
      pageModulePaths->Js.Array2.push(pageModulePath)->ignore;

      dependencyToPageModulesDict->Js.Dict.set(
        dependency,
        pageModulePaths->uniqueStringArray,
      );
    };
  };

  pageModulesAndTheirDependencies->Js.Array2.forEach(
    ((pageModulePath, dependencies)) => {
    dependencies->Js.Array2.forEach(dependency =>
      updateDependencyToPageModulesDict(~dependency, ~pageModulePath)
    )
  });

  // We handle pageWrapper module as a dependency of the page's module.
  // If pageWrapper module changes we check what page modules depend on it and rebuild them.
  let pageWrapperModulePaths =
    pages->Belt.Array.keepMap(page => {
      switch (page.pageWrapper) {
      | None => None
      | Some(wrapper) =>
        // Page wrapper can import other modules and have dependencies as well.
        // This should be also handled.
        updateDependencyToPageModulesDict(
          ~dependency=wrapper.modulePath,
          ~pageModulePath=page.modulePath,
        );
        Some(wrapper.modulePath);
      }
    });

  let headCssModulePaths = headCssFileToPagesDict->Js.Dict.keys;

  let allDependencies = {
    // Should we make empty array and push everything there instead of multiple concats?
    let dependencies =
      dependencyToPageModulesDict
      ->Js.Dict.entries
      ->Js.Array2.map(((dependency, _pageModules)) => dependency);

    Js.Array2.concat(pageModulePaths, dependencies)
    ->Js.Array2.concat(pageWrapperModulePaths)
    ->Js.Array2.concat(headCssModulePaths);
  };

  logger.debug(() =>
    Js.log2("[Watcher] Initial watcher dependencies:\n", allDependencies)
  );

  let watcher = Chokidar.chokidar->Chokidar.watchFiles(allDependencies);

  watcher->Chokidar.onReady(() =>
    logger.info(() => Js.Console.timeEnd(durationLabel))
  );

  let rebuildQueueRef: ref(array(PageBuilder.page)) = ref([||]);

  let rebuildPages = () => {
    switch (rebuildQueueRef^) {
    | [||] => ()
    | pagesToRebuild =>
      logger.info(() => Js.log("[Watcher] Pages rebuild triggered..."));

      logger.debug(() =>
        Js.log2(
          "[Watcher] Passing pages to worker to rebuild:\n",
          pagesToRebuild->showPages,
        )
      );

      BuildPageWorkerHelpers.buildPagesWithWorkers(
        ~pageAppArtifact,
        ~buildWorkersCount,
        // TODO Here we probably should group pages to rebuild by globalValues (one globalValues per worker)
        ~pages=[|pagesToRebuild|],
        ~outputDir,
        ~melangeOutputDir,
        ~logger,
        ~globalEnvValues,
        ~exitOnPageBuildError=false,
        ~generatedFilesSuffix="",
      )
      ->Promise.map(_ => {
          logger.debug(() =>
            Js.log(
              "[Watcher] Pages rebuild success, updating dependencies to watch...",
            )
          );

          pagesToRebuild->Js.Array2.forEach(page => {
            let pageModulePath = page.modulePath;
            let newDependencies = getModuleDependencies(~pageModulePath);

            logger.debug(() => {
              Js.log2(
                "[Watcher] New dependencies of the module: ",
                pageModulePath,
              );
              logger.debug(() =>
                Js.log2("[Watcher] New dependencies are:\n", newDependencies)
              );
            });

            newDependencies->Js.Array2.forEach(dependency =>
              updateDependencyToPageModulesDict(~dependency, ~pageModulePath)
            );

            watcher->Chokidar.add(newDependencies);

            logger.debug(() =>
              Js.log2(
                "[Watcher] dependencyToPageModulesDict:\n",
                dependencyToPageModulesDict,
              )
            );
          });
        })
      ->ignore;

      rebuildQueueRef := [||];
    };
  };

  let rebuildPagesDebounced = Debounce.debounce(~delayMs=700, rebuildPages);

  let onChangeOrUnlink = filepath => {
    let pagesToRebuild =
      switch (modulePathToPagesDict->Js.Dict.get(filepath)) {
      | Some(pages) =>
        logger.debug(() =>
          Js.log2("[Watcher] Exact page module changed: ", filepath)
        );
        pages;
      | None =>
        switch (dependencyToPageModulesDict->Js.Dict.get(filepath)) {
        | Some(pageModules) =>
          logger.debug(() => {
            Js.log2("[Watcher] Dependency changed: ", filepath);
            Js.log2(
              "[Watcher] Should rebuild these page modules:\n",
              pageModules,
            );
          });
          let pages =
            pageModules
            ->Js.Array2.map(modulePath => {
                switch (modulePathToPagesDict->Js.Dict.get(modulePath)) {
                | Some(pages) => Some(pages)
                | None =>
                  logger.debug(() =>
                    Js.log2(
                      "[Watcher] [Warning] The following page module is missing in dict: ",
                      modulePath,
                    )
                  );
                  None;
                }
              })
            ->Belt.Array.keepMap(v => v)
            ->Belt.Array.concatMany;
          pages;
        | None =>
          switch (headCssFileToPagesDict->Js.Dict.get(filepath)) {
          | Some(pages) =>
            logger.debug(() =>
              Js.log2("[Watcher] Head CSS file changed: ", filepath)
            );
            pages;
          | None =>
            // Nothing depends on a changed file. We should remove it from watcher.
            logger.debug(() =>
              Js.log2(
                "[Watcher] [Warning] No pages depend on the file: ",
                filepath,
              )
            );
            watcher->Chokidar.unwatch([|filepath|]);
            [||];
          }
        }
      };

    let newRebuildQueue =
      Js.Array2.concat(pagesToRebuild, rebuildQueueRef^)->makeUniquePageArray;

    rebuildQueueRef := newRebuildQueue;

    logger.debug(() =>
      Js.log2(
        "[Watcher] Rebuild pages queue:\n",
        (rebuildQueueRef^)->showPages,
      )
    );

    rebuildPagesDebounced();
  };

  // With rescript/bucklescript, "change" event is triggered when JS file updated after compilation.
  // But with Melange, "unlink" event is triggered.
  watcher->Chokidar.onChange(filepath => {
    logger.debug(() => Js.log2("[Watcher] Chokidar.onChange: ", filepath));
    onChangeOrUnlink(filepath);
  });

  watcher->Chokidar.onUnlink(filepath => {
    logger.debug(() => Js.log2("[Watcher] Chokidar.onUnlink: ", filepath));
    onChangeOrUnlink(filepath);
  });
};
