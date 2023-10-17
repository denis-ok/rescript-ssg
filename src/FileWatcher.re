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
      ~projectRootDir: string,
      ~outputDir: string,
      ~melangeOutputDir: option(string),
      ~logger: Log.logger,
      ~globalEnvValues: array((string, string)),
      ~buildWorkersCount: option(int)=?,
      pages: array(array(PageBuilder.page)),
    )
    : unit => {
  let watcher = Chokidar.chokidar->Chokidar.watchFiles([||]);

  let pages = Array.flat1(pages);

  let durationLabel = "[Watcher] Watching for file changes... Startup duration";
  Js.Console.timeStart(durationLabel);

  // Dependency is a some import in page's main module (page.modulePath).
  // Multiple pages can depend on the same dependency.
  // The dict below maps dependency path to the array of page module paths.
  let dependencyToPageModulesDict = Js.Dict.empty();

  let updateDependencyToPageModulesDict = (~dependency, ~pageModulePath) => {
    switch (dependencyToPageModulesDict->Js.Dict.get(dependency)) {
    | None =>
      dependencyToPageModulesDict->Js.Dict.set(
        dependency,
        ([|pageModulePath|], Set.fromArray([|pageModulePath|])),
      )
    | Some((pageModulePaths, pageModulePathsSet)) =>
      switch (pageModulePathsSet->Set.has(pageModulePath)) {
      | true => ()
      | _false =>
        pageModulePaths->Js.Array2.push(pageModulePath)->ignore;
        dependencyToPageModulesDict->Js.Dict.set(
          dependency,
          (pageModulePaths, pageModulePathsSet->Set.add(pageModulePath)),
        );
      }
    };
  };

  let modulePathToPagesDict = Js.Dict.empty();
  let headCssFileToPagesDict = Js.Dict.empty();
  let pageWrapperModulePaths = [||];

  pages->Js.Array2.forEach(page => {
    // Multiple pages can use the same page module. The common case is localized pages.
    // We get modulePath -> array(pages) dict here.
    // Fill modulePathToPagesDict
    switch (modulePathToPagesDict->Js.Dict.get(page.modulePath)) {
    | None => modulePathToPagesDict->Js.Dict.set(page.modulePath, [|page|])
    | Some(pages) => pages->Js.Array2.push(page)->ignore
    };

    // Fill headCssFileToPagesDict
    page.headCssFilepaths
    ->Js.Array2.forEach(headCssFile => {
        switch (headCssFileToPagesDict->Js.Dict.get(headCssFile)) {
        | None =>
          headCssFileToPagesDict->Js.Dict.set(
            headCssFile,
            ([|page|], Set.fromArray([|page.path->PagePath.toString|])),
          )
        | Some((pages, pagePathsSet)) =>
          switch (pagePathsSet->Set.has(page.path->PagePath.toString)) {
          | true => ()
          | _false =>
            pages->Js.Array2.push(page)->ignore;
            headCssFileToPagesDict->Js.Dict.set(
              headCssFile,
              (pages, pagePathsSet->Set.add(page.path->PagePath.toString)),
            );
          }
        }
      });

    // We handle pageWrapper module as a dependency of the page's module.
    // If pageWrapper module changes we check what page modules depend on it and rebuild them.
    // Fill pageWrapperModulePaths
    switch (page.pageWrapper) {
    | None => ()
    | Some(wrapper) =>
      // Page wrapper can import other modules and have dependencies as well.
      // This should be also handled.
      pageWrapperModulePaths->Js.Array2.push(wrapper.modulePath)->ignore;
      updateDependencyToPageModulesDict(
        ~dependency=wrapper.modulePath,
        ~pageModulePath=page.modulePath,
      );
    };
  });

  let pageModulePaths = modulePathToPagesDict->Js.Dict.keys;

  let pageModulesAndTheirDependencies =
    pageModulePaths->Js.Array2.map(pageModulePath => {
      Esbuild.getModuleDependencies(
        // Initial watcher start and getModuleDependencies must perform successfully.
        ~exitOnError=true,
        ~projectRootDir,
        ~modulePath=pageModulePath,
      )
      ->Promise.map(pageDependencies => (pageModulePath, pageDependencies))
    });

  let _: Js.Promise.t(unit) =
    pageModulesAndTheirDependencies
    ->Promise.all
    ->Promise.map(pageModulesAndTheirDependencies => {
        // Fill dependencyToPageModulesDict
        pageModulesAndTheirDependencies->Js.Array2.forEach(
          ((pageModulePath, pageDependencies)) => {
          pageDependencies->Js.Array2.forEach(dependency =>
            updateDependencyToPageModulesDict(~dependency, ~pageModulePath)
          )
        });

        let allDependencies = {
          let dependencies = [||];

          headCssFileToPagesDict
          ->Js.Dict.keys
          ->Js.Array2.forEach(headCssPath =>
              dependencies->Js.Array2.push(headCssPath)->ignore
            );

          dependencyToPageModulesDict
          ->Js.Dict.keys
          ->Js.Array2.forEach(dependencyPath =>
              dependencies->Js.Array2.push(dependencyPath)->ignore
            );

          pageModulePaths->Js.Array2.forEach(pageModulePath =>
            dependencies->Js.Array2.push(pageModulePath)->ignore
          );

          pageWrapperModulePaths->Js.Array2.forEach(pageWrapperModulePath =>
            dependencies->Js.Array2.push(pageWrapperModulePath)->ignore
          );

          dependencies;
        };

        watcher->Chokidar.add(allDependencies);

        logger.info(() => {
          Js.log("[Watcher] Initial dependencies collected");
          Js.Console.timeEnd(durationLabel);
        });

        logger.debug(() =>
          Js.log2(
            "[Watcher] Initial watcher dependencies:\n",
            allDependencies,
          )
        );
      });

  let rebuildQueueRef: ref(array(PageBuilder.page)) = ref([||]);

  let rebuildPages = (): Js.Promise.t(unit) => {
    switch (rebuildQueueRef^) {
    | [||] => Promise.resolve()
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
      ->Promise.flatMap(_ => {
          logger.debug(() =>
            Js.log(
              "[Watcher] Pages rebuild success, updating dependencies to watch...",
            )
          );

          let updatedPageModulesAndTheirDependencies =
            pagesToRebuild->Js.Array2.map(page => {
              let pageModulePath = page.modulePath;

              Esbuild.getModuleDependencies(
                // While compiler is working, this operation may fail.
                // It will return an empty array in this case, so we can ignore the error.
                // Maybe we should add some logic to make one more attempt to get dependencies.
                ~exitOnError=false,
                ~projectRootDir,
                ~modulePath=pageModulePath,
              )
              ->Promise.map(pageDependencies =>
                  (pageModulePath, pageDependencies)
                );
            });

          let newDependencies =
            updatedPageModulesAndTheirDependencies
            ->Promise.all
            ->Promise.map(pageModulesAndTheirDependencies => {
                pageModulesAndTheirDependencies->Js.Array2.map(
                  ((pageModulePath, pageDependencies)) => {
                  logger.debug(() => {
                    Js.log(
                      {j|[Watcher] Dependencies of updated page module: $(pageModulePath) are: $(pageDependencies)|j},
                    )
                  });

                  pageDependencies->Js.Array2.forEach(dependency =>
                    updateDependencyToPageModulesDict(
                      ~dependency,
                      ~pageModulePath,
                    )
                  );

                  pageDependencies;
                })
              });

          newDependencies->Promise.map(newDependencies => {
            let newDependencies = newDependencies->Array.flat1;

            watcher->Chokidar.add(newDependencies);

            logger.debug(() => {
              Js.log2(
                "[Watcher] Pages are rebuilded, dependencyToPageModulesDict:\n",
                dependencyToPageModulesDict,
              )
            });

            logger.info(() => {
              Js.log(
                "[Watcher] Pages are rebuilded, files to watch are updated",
              )
            });

            rebuildQueueRef := [||];
          });
        });
    };
  };

  let rebuildPagesDebounced =
    Debounce.debounce(~delayMs=1500, () => rebuildPages()->ignore);

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
        | Some((pageModules, _pageModulesSet)) =>
          logger.debug(() => {
            Js.log2("[Watcher] Dependency changed: ", filepath);
            Js.log2(
              "[Watcher] Should rebuild these page modules:\n",
              pageModules,
            );
          });

          let pages =
            pageModules
            ->Belt.Array.keepMap(modulePath => {
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
            ->Array.flat1;

          pages;
        | None =>
          switch (headCssFileToPagesDict->Js.Dict.get(filepath)) {
          | Some((pages, _pagePathsSet)) =>
            logger.debug(() =>
              Js.log2("[Watcher] Head CSS file changed: ", filepath)
            );
            pages;
          | None =>
            // Nothing depends on a changed file. We should remove it from watcher.
            watcher->Chokidar.unwatch([|filepath|]);

            logger.debug(() =>
              Js.log2(
                "[Watcher] [Warning] No pages depend on the file: ",
                filepath,
              )
            );

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
