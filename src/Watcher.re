let dirname = Utils.getDirname();

let makeUniqueArray = array => Set.fromArray(array)->Set.toArray;

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
        headCss: page.headCss,
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

// To make watcher work properly we need to:
// Monitor changes in a module itself and monitor changes in all dependencies of a module (except node modules?)
// After a module changed should we refresh dependencies and remove stale?

let startWatcher = (~outputDir, pages: array(PageBuilder.page)) => {
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

  let pageModulePaths =
    modulePathToPagesDict
    ->Js.Dict.entries
    ->Belt.Array.keepMap(((_, pages)) =>
        pages->Belt.Array.get(0)->Belt.Option.map(page => page.modulePath)
      );

  let modulesAndDependencies =
    pageModulePaths->Js.Array2.map(modulePath => {
      let dependencies = getModuleDependencies(~modulePath);
      (modulePath, dependencies);
    });

  let dependencyToPageModuleDict = Js.Dict.empty();

  let updateDependencyToPageModuleDict = (~dependency, ~modulePath) => {
    switch (dependencyToPageModuleDict->Js.Dict.get(dependency)) {
    | None =>
      dependencyToPageModuleDict->Js.Dict.set(dependency, [|modulePath|])
    | Some(pageModules) =>
      dependencyToPageModuleDict->Js.Dict.set(
        dependency,
        Js.Array2.concat([|modulePath|], pageModules)->makeUniqueArray,
      )
    };
  };

  modulesAndDependencies->Js.Array2.forEach(((modulePath, dependencies)) => {
    dependencies->Js.Array2.forEach(dependency =>
      updateDependencyToPageModuleDict(~dependency, ~modulePath)
    )
  });

  let pageWrapperModuleDependencies =
    pages->Belt.Array.keepMap(page => {
      switch (page.pageWrapper) {
      | None => None
      | Some(wrapper) =>
        let () =
          updateDependencyToPageModuleDict(
            ~dependency=wrapper.modulePath,
            ~modulePath=page.modulePath,
          );
        Some(wrapper.modulePath);
      }
    });

  let allDependencies = {
    let dependencies =
      dependencyToPageModuleDict
      ->Js.Dict.entries
      ->Js.Array2.map(((dependency, _pageModules)) => dependency);

    Js.Array2.concat(pageModulePaths, dependencies)
    ->Js.Array2.concat(pageWrapperModuleDependencies);
  };

  Js.log2("[Watcher] Initial watcher dependencies: ", allDependencies);

  let watcher = Chokidar.chokidar->Chokidar.watchFiles(allDependencies);

  let rebuildQueueRef: ref(array(string)) = ref([||]);

  watcher->Chokidar.onChange(filepath => {
    Js.log2("[Watcher] File changed: ", filepath);

    switch (modulePathToPagesDict->Js.Dict.get(filepath)) {
    | Some(_) =>
      Js.log2("[Watcher] Exact page module changed:", filepath);
      let newQueue =
        Js.Array2.concat([|filepath|], rebuildQueueRef^)->makeUniqueArray;
      rebuildQueueRef := newQueue;
    | None =>
      switch (dependencyToPageModuleDict->Js.Dict.get(filepath)) {
      | None =>
        // Nothing depends on changed file. Should we remove it from watcher?
        Js.log2("[Watcher] No pages depend on file:", filepath)
      | Some(pageModules) =>
        // Page dependency changed. Should rebuild pages that depend on it.
        Js.log2("[Watcher] Should rebuild these pages:", pageModules);
        let newQueue =
          Js.Array2.concat(pageModules, rebuildQueueRef^)->makeUniqueArray;
        rebuildQueueRef := newQueue;
      }
    };
  });

  let _intervalId =
    Js.Global.setInterval(
      () => {
        switch (rebuildQueueRef^) {
        | [||] => ()
        | rebuildQueue =>
          Js.log2("[Watcher] Page modules to rebuild queue: ", rebuildQueue);

          let pagesToRebuild =
            rebuildQueue
            ->Js.Array2.map(modulePath => {
                switch (modulePathToPagesDict->Js.Dict.get(modulePath)) {
                | None =>
                  Js.log2(
                    "[Watcher] Can't rebuild page, page module is missing in dict: ",
                    modulePath,
                  );
                  None;
                | Some(pages) => Some(pages)
                }
              })
            ->Belt.Array.keepMap(v => v)
            ->Belt.Array.concatMany;

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
