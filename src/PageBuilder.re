module ChildProcess = {
  [@module "child_process"]
  external execSync: (. string, Js.t('a)) => int = "execSync";

  module Error = {
    [@get] external stdout: Js.Exn.t => string = "stdout";
  };
};

[@val] external import_: string => Js.Promise.t('a) = "import";

// Node caches imported modules, here is a workaround, but there is a possible memory leak:
// https://ar.al/2021/02/22/cache-busting-in-node.js-dynamic-esm-imports/
// Also: https://github.com/nodejs/modules/issues/307

let freshImport = modulePath => {
  let timestamp = Js.Date.now()->Js.Float.toString;
  let cacheBustingModulePath = {j|$(modulePath)?update=$(timestamp)|j};
  import_(cacheBustingModulePath);
};

let defaultRoot = {js|<div id="app"></div>|js};

let makeDefaultRootWithRenderedData = (data: string) => {j|<div id="app">$(data)</div>|j};

let makeHtmlTemplate = (helmet: ReactHelmet.helmetInstance, renderedHtml) => {
  let htmlAttributes = helmet.htmlAttributes.toString();
  let title = helmet.title.toString();
  let meta = helmet.meta.toString();
  let link = helmet.link.toString();
  let bodyAttributes = helmet.bodyAttributes.toString();
  {j|
<!doctype html>
<html $(htmlAttributes)>
  <head>
    <meta charset="utf-8"/>
    $(title)
    $(meta)
    $(link)
  </head>
  <body $(bodyAttributes)>
    <div id="app">$(renderedHtml)</div>
  </body>
</html>
|j};
};

let defaultReactRootName = "elementString";

let reactRootTemplate = {js|
switch (ReactDOM.querySelector("#app")) {
| Some(root) => ReactDOM.hydrate(elementString, root)
| None => ()
};
|js};

type componentWithData('a) = {
  component: 'a => React.element,
  data: 'a,
};

type component =
  | ComponentWithoutData(React.element)
  | ComponentWithData(componentWithData('a)): component;

type wrapperComponent =
  | ComponentWithChildrenOnly(React.element => React.element);

type pageWrapper = {
  component: wrapperComponent,
  modulePath: string,
};

type page = {
  pageWrapper: option(pageWrapper),
  component,
  modulePath: string,
  path: PageBuilderT.PagePath.t,
};

let dataPropName = "data";

let makeReactAppModuleName = (~pagePath, ~moduleName) => {
  let modulePrefix =
    pagePath
    ->Js.String2.replaceByRe([%re "/\\//g"], "")
    ->Js.String2.replaceByRe([%re "/-/g"], "")
    ->Js.String2.replaceByRe([%re "/\\./g"], "");

  modulePrefix ++ moduleName ++ "App";
};

let buildPageHtmlAndReactApp = (~outputDir, page: page) => {
  let moduleName = Utils.getModuleNameFromModulePath(page.modulePath);

  let pagePath = page.path->PageBuilderT.PagePath.toString;

  let pageOutputDir = Path.join2(outputDir, pagePath);

  Js.log2(
    "[PageBuilder.buildPageHtmlAndReactApp] Output dir for page: ",
    pageOutputDir,
  );

  let () = {
    Fs.mkDirSync(pageOutputDir, {recursive: true});
  };

  let (element, elementString) = {
    switch (page.component) {
    | ComponentWithoutData(element) => (element, "<" ++ moduleName ++ " />")
    | ComponentWithData({component, data}) =>
      // We need a way to take a prop value of any type and inject it to generated React app template.
      // We take a prop and inject it's JSON stringified->parsed value in combination with Obj.magic.
      // This is unsafe part. Prop value should contain only values that possible to JSON.stringify<->JSON.parse.
      // So it should be composed only of simple values. Types like functions, dates, promises etc can't be stringified.
      let unsafeStringifiedPropValue =
        switch (data->Js.Json.stringifyAny) {
        | Some(propValueString) => {j|{{js|$(propValueString)|js}->Js.Json.parseExn->Obj.magic}|j}
        | None =>
          // Js.Json.stringifyAny(None) returns None. No need to do anything with it, can be injected to template as is.
          "None"
        };

      let element = component(data);
      let elementString =
        "<"
        ++ moduleName
        ++ " "
        ++ dataPropName
        ++ "="
        ++ unsafeStringifiedPropValue
        ++ " />";

      (element, elementString);
    };
  };

  let (element, elementString) = {
    switch (page.pageWrapper) {
    | None => (element, elementString)
    | Some({component, modulePath}) =>
      let moduleName = Utils.getModuleNameFromModulePath(modulePath);
      switch (component) {
      | ComponentWithChildrenOnly(f) =>
        let wrapperOpenTag = "<" ++ moduleName ++ ">";
        let wrapperCloseTag = "</" ++ moduleName ++ ">";
        let elementString = wrapperOpenTag ++ elementString ++ wrapperCloseTag;
        let wrappedElement = f(element);
        (wrappedElement, elementString);
      };
    };
  };

  let html = ReactDOMServer.renderToString(element);

  let htmlWithStyles = Emotion.Server.renderStylesToString(html);

  let helmet = ReactHelmet.renderStatic();

  let resultHtml = makeHtmlTemplate(helmet, htmlWithStyles);

  let resultReactApp =
    reactRootTemplate->Js.String2.replace(
      defaultReactRootName,
      elementString,
    );

  let pageAppModuleName = makeReactAppModuleName(~pagePath, ~moduleName);

  let resultHtmlPath = Path.join2(pageOutputDir, "index.html");

  let () = {
    let reactAppFilename = pageAppModuleName ++ ".re";
    Fs.writeFileSync(resultHtmlPath, resultHtml);
    Fs.writeFileSync(
      Path.join2(pageOutputDir, reactAppFilename),
      resultReactApp,
    );
  };

  let () = {
    let compiledReactAppFilename = pageAppModuleName ++ ".bs.js";
    let webpackPage: Webpack.page = {
      path: page.path,
      entryPath: Path.join2(pageOutputDir, compiledReactAppFilename),
      outputDir: pageOutputDir,
      htmlTemplatePath: resultHtmlPath,
    };
    Webpack.pages->Js.Dict.set(pageAppModuleName, webpackPage);
  };

  Js.log2(
    "[PageBuilder.buildPageHtmlAndReactApp] Build finished: ",
    moduleName,
  );
};

let makeUniqueArray = array => Set.fromArray(array)->Set.toArray;

let dirname = Utils.getDirname();

let rebuildPagesWithWorker = (~outputDir, pages: array(page)) => {
  let rebuildPages =
    pages->Js.Array2.map(page => {
      let rebuildPage: RebuildPageWorkerT.rebuildPage = {
        pageWrapper: {
          switch (page.pageWrapper) {
          | None => None
          | Some({component: ComponentWithChildrenOnly(_), modulePath}) =>
            Some({component: ComponentWithChildrenOnly, modulePath})
          };
        },
        component: {
          switch (page.component) {
          | ComponentWithoutData(_) => ComponentWithoutData
          | ComponentWithData({data, _}) => ComponentWithData({data: data})
          };
        },
        modulePath: page.modulePath,
        outputDir,
        path: page.path,
      };

      rebuildPage;
    });

  WorkingThreads.runWorker(
    ~workerModulePath=Path.join2(dirname, "RebuildPageWorker.bs.js"),
    ~workerData=rebuildPages,
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

let startWatcher = (~outputDir, pages: list(page)) => {
  let modulePathToPagesDict = Js.Dict.empty();
  pages->Belt.List.forEach(page => {
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
    pages
    ->Belt.List.keepMap(page => {
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
      })
    ->Belt.List.toArray;

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

let buildPages = (~outputDir, pages: list(page)) => {
  Js.log("[PageBuilder.buildPages] Building pages...");

  let pagesDict = Js.Dict.empty();

  let () =
    pages->Belt.List.forEach(page => {
      let pagePath = PageBuilderT.PagePath.toString(page.path);
      switch (pagesDict->Js.Dict.get(pagePath)) {
      | None => pagesDict->Js.Dict.set(pagePath, page)
      | Some(_) =>
        Js.log3(
          "[PageBuilder.buildPages] List of pages contains pages with the same paths. Page with path: ",
          page.path,
          " has already been built.",
        );

        Process.exit(1);
      };

      buildPageHtmlAndReactApp(~outputDir, page);
    });

  pagesDict;
};

let start = (~pages: list(page), ~outputDir, ~webpackOutputDir, ~mode) => {
  let _pagesDict = buildPages(~outputDir, pages);

  startWatcher(~outputDir, pages);

  Webpack.startDevServer(~mode, ~webpackOutputDir);
};

let build =
    (~pages, ~outputDir, ~webpackOutputDir, ~rescriptBinaryPath, ~mode) => {
  let _pagesDict = buildPages(~outputDir, pages);

  Js.log("[PageBuilder.build] Compiling React app files...");

  switch (ChildProcess.execSync(. rescriptBinaryPath, {"encoding": "utf8"})) {
  | exception (Js.Exn.Error(error)) =>
    Js.log2(
      "[PageBuilder.build] Rescript build failed:\n",
      error->Js.Exn.message,
    );
    Js.log2(
      "[PageBuilder.build] Rescript build failed:\n",
      error->ChildProcess.Error.stdout,
    );
    Process.exit(1);
  | stdout => Js.log2("[PageBuilder.build] Rescript build success:\n", stdout)
  };

  Js.log("[PageBuilder.build] Building webpack bundle...");

  Webpack.build(~mode, ~webpackOutputDir, ~verbose=true);
};
