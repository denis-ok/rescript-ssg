let uniqueStringArray = (array: array(string)) =>
  Set.fromArray(array)->Set.toArray;

let uniqueArray = (array: array('a), ~getId: 'a => string) => {
  let items = array->Js.Array2.map(v => (v->getId, v));
  items->Js.Dict.fromArray->Js.Dict.values;
};

let uniquePageArray = (pages: array(PageBuilder.page)) => {
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

let getModuleDependencies = (~modulePath) =>
  DependencyTree.makeList({
    filename: modulePath,
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
// If the change is in a root module -> simply get pages from modulePathToPagesDict and rebuild them.
// If the change is in a dependency -> get root modules from a dependency -> get pages from root modules.
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

  // Multiple pages can use the same root module. The common case is localized pages.
  // We get modulePath -> array(pages) dict here.
  let modulePathToPagesDict = Js.Dict.empty();
  pages->Js.Array2.forEach(page => {
    switch (modulePathToPagesDict->Js.Dict.get(page.modulePath)) {
    | None => modulePathToPagesDict->Js.Dict.set(page.modulePath, [|page|])
    | Some(pages) =>
      modulePathToPagesDict->Js.Dict.set(
        page.modulePath,
        Js.Array2.concat([|page|], pages),
      )
    }
  });

  let pageModulePaths = modulePathToPagesDict->Js.Dict.keys;

  let modulesAndDependencies =
    pageModulePaths->Js.Array2.map(modulePath => {
      let dependencies = getModuleDependencies(~modulePath);
      (modulePath, dependencies);
    });

  // Dependency is a dependency of the root module (modulePath).
  // Multiple root modules can depend on the same dependency.
  // The dict below maps dependency to the array of root modules.

  let dependencyToPageModulesDict = Js.Dict.empty();

  let updateDependencyToPageModulesDict = (~dependency, ~modulePath) => {
    switch (dependencyToPageModulesDict->Js.Dict.get(dependency)) {
    | None =>
      dependencyToPageModulesDict->Js.Dict.set(dependency, [|modulePath|])
    | Some(pageModules) =>
      dependencyToPageModulesDict->Js.Dict.set(
        dependency,
        Js.Array2.concat([|modulePath|], pageModules)->uniqueStringArray,
      )
    };
  };

  modulesAndDependencies->Js.Array2.forEach(((modulePath, dependencies)) => {
    dependencies->Js.Array2.forEach(dependency =>
      updateDependencyToPageModulesDict(~dependency, ~modulePath)
    )
  });

  // We handle pageWrapper module as a dependency of the root module.
  // So if pageWrapper module changes we check what root modules (pages) depend on it and rebuild them.

  let pageWrapperModuleDependencies =
    pages->Belt.Array.keepMap(page => {
      switch (page.pageWrapper) {
      | None => None
      | Some(wrapper) =>
        updateDependencyToPageModulesDict(
          ~dependency=wrapper.modulePath,
          ~modulePath=page.modulePath,
        );
        Some(wrapper.modulePath);
      }
    });

  let headCssFileToPagesDict = Js.Dict.empty();
  pages->Js.Array2.forEach(page => {
    page.headCssFilepaths
    ->Js.Array2.forEach(headCssFile => {
        switch (headCssFileToPagesDict->Js.Dict.get(headCssFile)) {
        | None => headCssFileToPagesDict->Js.Dict.set(headCssFile, [|page|])
        | Some(pages) =>
          headCssFileToPagesDict->Js.Dict.set(
            headCssFile,
            Js.Array2.concat([|page|], pages)->uniquePageArray,
          )
        }
      })
  });

  let headCssDependencies = headCssFileToPagesDict->Js.Dict.keys;

  let allDependencies = {
    let dependencies =
      dependencyToPageModulesDict
      ->Js.Dict.entries
      ->Js.Array2.map(((dependency, _pageModules)) => dependency);

    Js.Array2.concat(pageModulePaths, dependencies)
    ->Js.Array2.concat(pageWrapperModuleDependencies)
    ->Js.Array2.concat(headCssDependencies);
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
            let modulePath = page.modulePath;
            let newDependencies = getModuleDependencies(~modulePath);

            logger.debug(() => {
              Js.log2(
                "[Watcher] New dependencies of the module: ",
                modulePath,
              );
              logger.debug(() =>
                Js.log2("[Watcher] New dependencies are:\n", newDependencies)
              );
            });

            newDependencies->Js.Array2.forEach(dependency =>
              updateDependencyToPageModulesDict(~dependency, ~modulePath)
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
      Js.Array2.concat(pagesToRebuild, rebuildQueueRef^)->uniquePageArray;

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
