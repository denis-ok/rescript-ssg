let dirname = Utils.getDirname();

let uniqueStringArray = (array: array(string)) =>
  Set.fromArray(array)->Set.toArray;

let uniqueArray = (array: array('a), ~getId: 'a => string) => {
  let items = array->Js.Array2.map(v => (v->getId, v));
  items->Js.Dict.fromArray->Js.Dict.values;
};

let uniquePageArray = (pages: array(PageBuilder.page)) => {
  pages->uniqueArray(~getId=page =>
    PageBuilderT.PagePath.toString(page.path)
  );
};

let rebuildPagesWithWorker = (~outputDir, pages: array(PageBuilder.page)) => {
  let rebuildPages =
    pages->Js.Array2.map(page => {
      let rebuildPage: RebuildPageWorkerT.rebuildPage = {
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
        headCssFiles: page.headCssFiles,
        path: page.path,
      };

      rebuildPage;
    });

  let workerData: RebuildPageWorkerT.workerData = rebuildPages;

  WorkingThreads.runWorker(
    ~workerModulePath=Path.join2(dirname, "RebuildPageWorker.bs.js"),
    ~workerData,
  );
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
// 4. Watch for the changes in headCssFiles.

// After a module changes we should rebuild a page and refresh dependency dicts to remove the stale ones.

// The watcher logic looks like this:
// We detect change in some file.
// If the change is in a root module -> simply get pages from modulePathToPagesDict and rebuild them.
// If the change is in a dependency -> get root modules from a dependency -> get pages from root modules.
// If the change is in a head CSS file -> get pages that use this css file and rebuild them.

let startWatcher = (~outputDir, pages: array(PageBuilder.page)) => {
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

  let dependencyToPageModuleDict = Js.Dict.empty();

  let updateDependencyToPageModuleDict = (~dependency, ~modulePath) => {
    switch (dependencyToPageModuleDict->Js.Dict.get(dependency)) {
    | None =>
      dependencyToPageModuleDict->Js.Dict.set(dependency, [|modulePath|])
    | Some(pageModules) =>
      dependencyToPageModuleDict->Js.Dict.set(
        dependency,
        Js.Array2.concat([|modulePath|], pageModules)->uniqueStringArray,
      )
    };
  };

  modulesAndDependencies->Js.Array2.forEach(((modulePath, dependencies)) => {
    dependencies->Js.Array2.forEach(dependency =>
      updateDependencyToPageModuleDict(~dependency, ~modulePath)
    )
  });

  // We handle pageWrapper module as a dependency of the root module.
  // So if pageWrapper module changes we check what root modules (pages) depend on it and rebuild them.

  let pageWrapperModuleDependencies =
    pages->Belt.Array.keepMap(page => {
      switch (page.pageWrapper) {
      | None => None
      | Some(wrapper) =>
        updateDependencyToPageModuleDict(
          ~dependency=wrapper.modulePath,
          ~modulePath=page.modulePath,
        );
        Some(wrapper.modulePath);
      }
    });

  let headCssFileToPagesDict = Js.Dict.empty();
  pages->Js.Array2.forEach(page => {
    page.headCssFiles
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
      dependencyToPageModuleDict
      ->Js.Dict.entries
      ->Js.Array2.map(((dependency, _pageModules)) => dependency);

    Js.Array2.concat(pageModulePaths, dependencies)
    ->Js.Array2.concat(pageWrapperModuleDependencies)
    ->Js.Array2.concat(headCssDependencies);
  };

  Js.log2("[Watcher] Initial watcher dependencies: ", allDependencies);

  let watcher = Chokidar.chokidar->Chokidar.watchFiles(allDependencies);

  let rebuildQueueRef: ref(array(PageBuilder.page)) = ref([||]);

  watcher->Chokidar.onChange(filepath => {
    Js.log2("[Watcher] File changed: ", filepath);

    let updatedRebuildQueue =
      switch (modulePathToPagesDict->Js.Dict.get(filepath)) {
      | Some(pages) =>
        Js.log2("[Watcher] Exact page module changed: ", filepath);
        Js.Array2.concat(pages, rebuildQueueRef^)->uniquePageArray;
      | None =>
        switch (dependencyToPageModuleDict->Js.Dict.get(filepath)) {
        | Some(pageModules) =>
          Js.log2(
            "[Watcher] Should rebuild these page modules:",
            pageModules,
          );

          let pages =
            pageModules
            ->Js.Array2.map(modulePath => {
                switch (modulePathToPagesDict->Js.Dict.get(modulePath)) {
                | None =>
                  Js.log2(
                    "[Watcher] Page module is missing in dict: ",
                    modulePath,
                  );
                  None;
                | Some(pages) => Some(pages)
                }
              })
            ->Belt.Array.keepMap(v => v)
            ->Belt.Array.concatMany;

          Js.Array2.concat(pages, rebuildQueueRef^)->uniquePageArray;
        | None =>
          switch (headCssFileToPagesDict->Js.Dict.get(filepath)) {
          | Some(pages) =>
            Js.log("[Watcher] Head CSS file changed.");
            Js.Array2.concat(pages, rebuildQueueRef^)->uniquePageArray;
          | None =>
            // Nothing depends on changed file. We should remove it from watcher.
            Js.log2("[Watcher] No pages depend on file:", filepath);
            rebuildQueueRef^;
          }
        }
      };

    if (rebuildQueueRef^ != updatedRebuildQueue) {
      rebuildQueueRef := updatedRebuildQueue;
    };
  });

  let _intervalId =
    Js.Global.setInterval(
      () => {
        switch (rebuildQueueRef^) {
        | [||] => ()
        | pagesToRebuild =>
          Js.log2(
            "[Watcher] Pages modules to rebuild queue: ",
            pagesToRebuild,
          );

          rebuildPagesWithWorker(~outputDir, pagesToRebuild)
          ->Promise.map(_ => {
              Js.log("[Watcher] Updating dependencies to watch");

              pagesToRebuild->Js.Array2.forEach(page => {
                let modulePath = page.modulePath;
                let newDependencies = getModuleDependencies(~modulePath);

                Js.log3(
                  "[Watcher] New dependencies of the module ",
                  modulePath,
                  newDependencies,
                );

                newDependencies->Js.Array2.forEach(dependency =>
                  updateDependencyToPageModuleDict(~dependency, ~modulePath)
                );

                watcher->Chokidar.add(newDependencies);

                Js.log2(
                  "[Watcher] !!! dependencyToPageModuleDict",
                  dependencyToPageModuleDict,
                );
              });
            })
          ->ignore;

          rebuildQueueRef := [||];
        }
      },
      1500,
    );

  ();
};
