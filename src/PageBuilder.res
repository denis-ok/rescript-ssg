module Path = {
  @module("path") external join2: (string, string) => string = "join"
  @module("path") external join3: (string, string, string) => string = "join"
}

module Fs = {
  @module("fs") external readFileSync: (string, string) => string = "readFileSync"

  @module("fs") external writeFileSync: (string, string) => unit = "writeFileSync"

  type makeDirSyncOptions = {recursive: bool}

  @module("fs") external mkDirSync: (string, makeDirSyncOptions) => unit = "mkdirSync"

  type rmSyncOptions = {force: bool, recursive: bool}

  @module("fs") external rmSync: (string, rmSyncOptions) => unit = "rmSync"
}

module String = {
  let replace = Js.String2.replace
}

@val external import_: string => Js.Promise.t<'a> = "import"

// Node caches imported modules, here is a workaround, but there is a possible memory leak:
// https://ar.al/2021/02/22/cache-busting-in-node.js-dynamic-esm-imports/
// Also: https://github.com/nodejs/modules/issues/307
let freshImport = modulePath => {
  let timestamp = Js.Date.now()->Js.Float.toString
  let cacheBustingModulePath = `${modulePath}?update=${timestamp}`
  import_(cacheBustingModulePath)
}

let srcPath = SrcPath.srcPath

module Webpack = {
  type page = {
    title: string,
    slug: string,
    entryPath: string,
    outputDir: string,
    htmlTemplatePath: string,
  }

  let pages: Js.Dict.t<page> = Js.Dict.empty()
}

let defaultRoot = `<div id="app"></div>`

let makeDefaultRootWithRenderedData = data => `<div id="app">${data}</div>`

let htmlTemplate = Fs.readFileSync(Path.join2(srcPath, "default-template.html"), "utf8")

let userOutputDir: ref<option<string>> = ref(None)

let setOutputDir = path => {
  Js.log2("Output dir set to: ", path)
  userOutputDir.contents = Some(path)
}

let getOutputDir = () => {
  switch userOutputDir.contents {
  | None => Js.Exn.raiseError("[getOutputDir] Output dir wasn't set.")
  | Some(dir) => dir
  }
}

let defaultReactRootName = "RootComponent"

let reactRootTemplate = `
switch ReactDOM.querySelector("#app") {
| Some(root) => ReactDOM.hydrate(<${defaultReactRootName} />, root)
| None => ()
}
`

type page = {
  component: React.element,
  moduleName: string,
  modulePath: string,
  slug: string,
  path: string,
}

let pages: Js.Dict.t<page> = Js.Dict.empty()

module Log = {
  let log = (scope, msg) => {
    let scope = `[${scope}]: `
    Js.log(scope ++ msg)
  }

  let buildPage = log("buildPage")

  let buildPage2 = (msg1, msg2) => log("buildPage", msg1 ++ msg2)

  let watcher = log("watcher")

  let watcher2 = (msg1, msg2) => watcher(msg1 ++ msg2)
}

let indexHtmlFilename = "index.html"

let buildPage = (page: page) => {
  let {component: _, moduleName, slug, path, modulePath} = page

  let pageOutputDir = Path.join2(getOutputDir(), path)

  Log.buildPage2("Output dir for page: ", pageOutputDir)

  let resultHtmlPath = Path.join2(pageOutputDir, indexHtmlFilename)

  let resultReactApp = reactRootTemplate->String.replace(defaultReactRootName, moduleName)

  let resultReactRescriptAppFilename = moduleName ++ "App.res"

  let resultReactCompiledAppFilename = moduleName ++ "App.bs.js"

  let () = {
    Fs.mkDirSync(pageOutputDir, {recursive: true})
  }

  let () = {
    freshImport(modulePath)
    ->Promise.map(module_ => {
      // Js.log2("imported module: ", module_)
      let component: unit => React.element = module_["make"]
      let renderedComponent = ReactDOMServer.renderToString(component())
      let resultHtml =
        htmlTemplate->String.replace(
          defaultRoot,
          makeDefaultRootWithRenderedData(renderedComponent),
        )

      let () = {
        Fs.writeFileSync(resultHtmlPath, resultHtml)
        Fs.writeFileSync(Path.join2(pageOutputDir, resultReactRescriptAppFilename), resultReactApp)
      }
    })
    ->ignore
  }

  let () = {
    let webpackPage: Webpack.page = {
      title: moduleName,
      slug: slug,
      entryPath: Path.join2(pageOutputDir, resultReactCompiledAppFilename),
      outputDir: pageOutputDir,
      htmlTemplatePath: resultHtmlPath,
    }
    Webpack.pages->Js.Dict.set(moduleName, webpackPage)
  }

  let () = {
    switch pages->Js.Dict.get(moduleName) {
    | None => pages->Js.Dict.set(moduleName, page)
    | Some(_) => ()
    }
  }

  Log.buildPage2("Build finished: ", moduleName)
}

let buildJsonWithWebpackPages = () => {
  let json = Webpack.pages->Js.Dict.values->Js.Json.serializeExn
  Fs.writeFileSync(Path.join2(getOutputDir(), "pages.json"), json)
}

let startWatcher = () => {
  if true {
    let pagesPaths = pages->Js.Dict.entries->Js.Array2.map(((_, page)) => page.modulePath)

    let watcher = Chokidar.chokidar->Chokidar.watchFiles(pagesPaths)
    watcher->Chokidar.onChange(filepath => {
      Log.watcher2("File changed: ", filepath)

      let moduleName =
        filepath
        ->Js.String2.replace(".bs.js", "")
        ->Js.String2.split("/")
        ->Belt.List.fromArray
        ->Belt.List.reverse
        ->Belt.List.head

      switch moduleName {
      | None => Log.watcher("Can't rebuild page, moduleName is None")
      | Some("") => Log.watcher("Can't rebuild page, moduleName is empty string")
      | Some(moduleName) =>
        switch pages->Js.Dict.get(moduleName) {
        | None => Log.watcher2("Can't rebuild page, data is missing: ", moduleName)
        | Some(page) =>
          Log.watcher2("Rebuilding page: ", moduleName)
          buildPage(page)
        }
      }
    })
  }
}
