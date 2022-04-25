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

let srcPath = Utils.srcPath

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

let buildPage = (page: page) => {
  let {component, moduleName, slug, path} = page

  let pageOutputDir = Path.join2(getOutputDir(), path)
  Js.log2("Output dir for page: ", pageOutputDir)

  let renderedComponent = ReactDOMServer.renderToString(component)

  let resultHtml =
    htmlTemplate->String.replace(defaultRoot, makeDefaultRootWithRenderedData(renderedComponent))
  let resultHtmlFilename = "index.html"
  let resultHtmlPath = Path.join2(pageOutputDir, resultHtmlFilename)

  let resultReactApp = reactRootTemplate->String.replace(defaultReactRootName, moduleName)
  let resultReactRescriptAppFilename = moduleName ++ "App.res"
  let resultReactCompiledAppFilename = moduleName ++ "App.bs.js"

  let () = {
    Fs.mkDirSync(pageOutputDir, {recursive: true})
    Fs.writeFileSync(resultHtmlPath, resultHtml)
    Fs.writeFileSync(Path.join2(pageOutputDir, resultReactRescriptAppFilename), resultReactApp)
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

  Js.log2("Page build finished: ", moduleName)
}

let buildJsonWithWebpackPages = () => {
  let json = Webpack.pages->Js.Dict.values->Js.Json.serializeExn
  Fs.writeFileSync(Path.join2(getOutputDir(), "pages.json"), json)
}

let startWatcher = () => {
  if true {
    let pagesPaths = pages->Js.Dict.entries->Js.Array2.map(((_, page)) => page.modulePath)

    let watcher = Chokidar.chokidar->Chokidar.watchFiles(pagesPaths)
    watcher->Chokidar.on("all", (event, filepath) => {
      Js.log3(Js.Date.make(), "file event: ", event)
      Js.log2("file path: ", filepath)

      let filename =
        filepath
        ->Js.String2.replace(".bs.js", "")
        ->Js.String2.split("/")
        ->Belt.List.fromArray
        ->Belt.List.reverse
        ->Belt.List.head

      let () = switch filename {
      | None => ()
      | Some("") => ()
      | Some(moduleName) =>
        Js.log2("Trying to rebuild page: ", moduleName)

        switch pages->Js.Dict.get(moduleName) {
        | None => Js.Console.error2("Can't rebuild page, page data is missing: ", moduleName)
        | Some(page) =>
          Js.log2("Rebuilding page: ", moduleName)
          buildPage(page)
        }
      }
    })
  }
}
