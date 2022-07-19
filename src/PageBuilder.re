module Fs = {
  [@module "fs"]
  external readFileSync: (string, string) => string = "readFileSync";

  [@module "fs"]
  external writeFileSync: (string, string) => unit = "writeFileSync";

  type makeDirSyncOptions = {recursive: bool};

  [@module "fs"]
  external mkDirSync: (string, makeDirSyncOptions) => unit = "mkdirSync";

  type rmSyncOptions = {
    force: bool,
    recursive: bool,
  };

  [@module "fs"] external rmSync: (string, rmSyncOptions) => unit = "rmSync";
};

module ChildProcess = {
  [@module "child_process"]
  external execSync: (. string, Js.t('a)) => int = "execSync";

  module Error = {
    [@get] external stdout: Js.Exn.t => string = "stdout";
  };
};

// [@val] external import_: string => Js.Promise.t('a) = "import";

// Node caches imported modules, here is a workaround, but there is a possible memory leak:
// https://ar.al/2021/02/22/cache-busting-in-node.js-dynamic-esm-imports/
// Also: https://github.com/nodejs/modules/issues/307

// let freshImport = modulePath => {
//   let timestamp = Js.Date.now()->Js.Float.toString;
//   let cacheBustingModulePath = {j|$(modulePath)?update=$(timestamp)|j};
//   import_(cacheBustingModulePath);
// };

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

type wrapper1('a) = {
  wrapper: (React.element, 'a) => React.element,
  wrapperReference: string,
  arg: 'a,
  argReference: string,
};

type wrapper('a) =
  | Wrapper1(wrapper1('a));

type page('a) = {
  wrapper: option(wrapper('a)),
  component: React.element,
  moduleName: string,
  modulePath: string,
  path: string,
};

let indexHtmlFilename = "index.html";

let applyWrapper1 =
    (
      ~page: page('a),
      ~wrapper: (React.element, 'a) => React.element,
      ~arg: 'a,
      ~wrapperReference: string,
      ~argReference: string,
    ) => {
  let {component, moduleName, _} = page;
  let reactElement = wrapper(component, arg);
  let componentString = {j|$(wrapperReference)(<$(moduleName) />, $(argReference))|j};
  (reactElement, componentString);
};

let buildPageHtmlAndReactApp = (~outputDir, page: page('a)) => {
  let {component, moduleName, path: pagePath, wrapper, _} = page;

  let pageOutputDir = Path.join2(outputDir, pagePath);

  Js.log2(
    "[PageBuilder.buildPageHtmlAndReactApp] Output dir for page: ",
    pageOutputDir,
  );

  let resultHtmlPath = Path.join2(pageOutputDir, indexHtmlFilename);

  let () = {
    Fs.mkDirSync(pageOutputDir, {recursive: true});
  };

  let (element, elementString) = {
    switch (wrapper) {
    | None => (component, "<" ++ moduleName ++ " />")
    | Some(Wrapper1({wrapper, arg, wrapperReference, argReference})) =>
      let (element, elementString) =
        applyWrapper1(
          ~page,
          ~wrapper,
          ~arg,
          ~wrapperReference,
          ~argReference,
        );
      (element, elementString);
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

  let () = {
    let resultReactRescriptAppFilename = moduleName ++ "App.re";
    Fs.writeFileSync(resultHtmlPath, resultHtml);
    Fs.writeFileSync(
      Path.join2(pageOutputDir, resultReactRescriptAppFilename),
      resultReactApp,
    );
  };

  let () = {
    let resultReactCompiledAppFilename = moduleName ++ "App.bs.js";
    let webpackPage: Webpack.page = {
      title: moduleName,
      path: pagePath,
      entryPath: Path.join2(pageOutputDir, resultReactCompiledAppFilename),
      outputDir: pageOutputDir,
      htmlTemplatePath: resultHtmlPath,
    };
    Webpack.pages->Js.Dict.set(moduleName, webpackPage);
  };

  Js.log2(
    "[PageBuilder.buildPageHtmlAndReactApp] Build finished: ",
    moduleName,
  );
};

// To make watcher work properly we need to:
// Monitor changes in a module itself and monitor changes in all dependencies of a module (except node modules?)
// After a module changed! We should refresh dependencies of a module?
// Finally we should have dependency -> list(pageModule) dict?

module Set = {
  type t('a);
  [@new] external fromArray: array('a) => t('a) = "Set";

  type arrayModule;

  [@val] external arrayModule: arrayModule = "Array";
  [@send] external arrayFrom: (arrayModule, t('a)) => array('a) = "from";

  let toArray = set => arrayModule->arrayFrom(set);
};

let makeUniqueArray = array => Set.fromArray(array)->Set.toArray;

let startWatcher = (pages: list(page('a))) => {
  let modulePathToPageDict: Js.Dict.t(page('a)) = Js.Dict.empty();
  pages->Belt.List.forEach(page => {
    switch (modulePathToPageDict->Js.Dict.get(page.modulePath)) {
    | None => modulePathToPageDict->Js.Dict.set(page.modulePath, page)
    | Some(_) => ()
    }
  });

  let pagesPaths =
    modulePathToPageDict
    ->Js.Dict.entries
    ->Js.Array2.map(((_, page)) => page.modulePath);

  let modulesAndDependencies =
    pagesPaths->Js.Array2.map(modulePath => {
      let dependencies =
        DependencyTree.makeList({
          filename: modulePath,
          directory: "/Users/denis/projects/builder/",
          filter: path => path->Js.String2.indexOf("node_modules") == (-1),
        });

      (modulePath, dependencies);
    });

  let dependencyToPageModuleDict = Js.Dict.empty();
  modulesAndDependencies->Js.Array2.forEach(((modulePath, dependencies)) => {
    dependencies->Js.Array2.forEach(dep => {
      switch (dependencyToPageModuleDict->Js.Dict.get(dep)) {
      | None => dependencyToPageModuleDict->Js.Dict.set(dep, [|modulePath|])
      | Some(pageModules) =>
        dependencyToPageModuleDict->Js.Dict.set(
          dep,
          Js.Array.concat([|modulePath|], pageModules),
        )
      }
    })
  });

  let allDependencies = {
    let dependencies =
      dependencyToPageModuleDict
      ->Js.Dict.entries
      ->Js.Array2.map(((dependency, _pageModules)) => dependency);

    Js.Array.concat(pagesPaths, dependencies);
  };

  let watcher = Chokidar.chokidar->Chokidar.watchFiles(allDependencies);

  let rebuildQueueRef: ref(array(string)) = ref([||]);

  watcher->Chokidar.onChange(filepath => {
    Js.log2("[Watcher] File changed: ", filepath);

    switch (modulePathToPageDict->Js.Dict.get(filepath)) {
    | Some(_) =>
      // Exact page module changed
      Js.log2("[Watcher] Exact page module changed:", filepath);
      let queue =
        Js.Array2.concat([|filepath|], rebuildQueueRef^)->makeUniqueArray;
      rebuildQueueRef := queue;
    | None =>
      switch (dependencyToPageModuleDict->Js.Dict.get(filepath)) {
      | None =>
        // Nothing depends on changed file. Should remove it from watcher.
        Js.log2("[Watcher] No pages depend on file:", filepath)
      | Some(pageModules) =>
        // Page dependency changed. Should rebuild pages that depend on it.
        Js.log2("[Watcher] Should rebuild these pages:", pageModules);
        let queue =
          Js.Array2.concat(pageModules, rebuildQueueRef^)->makeUniqueArray;
        rebuildQueueRef := queue;
      }
    };
  });

  let _intervalId =
    Js.Global.setInterval(
      () => {
        let rebuildQueue = rebuildQueueRef^;
        if (Js.Array2.length(rebuildQueue) != 0) {
          Js.log2("[Watcher] Pages to rebuild queue: ", rebuildQueue);

          // rebuilding pages
          rebuildQueue->Js.Array2.forEach(modulePath => {
            switch (modulePathToPageDict->Js.Dict.get(modulePath)) {
            | Some(_page) =>
              Js.log2("[Watcher] Trying to rebuild page: ", modulePath)
            | None =>
              Js.log2(
                "[Watcher] Can't rebuild page, page module is missing in dict: ",
                modulePath,
              )
            }
          });

          // cleanup queue
          rebuildQueueRef := [||];
        };
      },
      1000,
    );

  ();
};

let buildPages = (~outputDir, pages: list(page('a))) => {
  Js.log("[PageBuilder.buildPages] Building pages...");

  let pagesDict = Js.Dict.empty();

  let () =
    pages->Belt.List.forEach(page => {
      switch (pagesDict->Js.Dict.get(page.path)) {
      | None => pagesDict->Js.Dict.set(page.path, page)
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

let start = (~pages: list(page('a)), ~outputDir, ~webpackOutputDir, ~mode) => {
  let _pagesDict = buildPages(~outputDir, pages);

  startWatcher(pages);

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
