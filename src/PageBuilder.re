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

// Watcher doesn't work properly because we need to monitor changes in all dependencies of a page
// let startWatcher = () =>
//   if (true) {
//     let pagesPaths =
//       pages->Js.Dict.entries->Js.Array2.map(((_, page)) => page.modulePath);

//     let watcher = Chokidar.chokidar->Chokidar.watchFiles(pagesPaths);
//     watcher->Chokidar.onChange(filepath => {
//       Log.watcher2("File changed: ", filepath);

//       let moduleName =
//         filepath
//         ->Js.String2.replace(".bs.js", "")
//         ->Js.String2.split("/")
//         ->Belt.List.fromArray
//         ->Belt.List.reverse
//         ->Belt.List.head;

//       switch (moduleName) {
//       | None => Log.watcher("Can't rebuild page, moduleName is None")
//       | Some("") =>
//         Log.watcher("Can't rebuild page, moduleName is empty string")
//       | Some(moduleName) =>
//         switch (pages->Js.Dict.get(moduleName)) {
//         | None =>
//           Log.watcher2("Can't rebuild page, data is missing: ", moduleName)
//         | Some(page) =>
//           Log.watcher2("Rebuilding page: ", moduleName);
//           buildPage(page);
//         }
//       };
//     });
//   };

let buildPages = (~outputDir, pages) => {
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
