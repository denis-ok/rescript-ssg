module Path = {
  @module("path") external join2: (string, string) => string = "join"
}

module Fs = {
  @module("fs") external readFileSync: (string, string) => string = "readFileSync"
  @module("fs") external writeFileSync: (string, string) => unit = "writeFileSync"
}

module String = {
  let replace = Js.String2.replace
}

let srcPath = Utils.srcPath

type page = {
  path: list<string>,
  component: unit => React.element,
}

let defaultRoot = `<div id="app"></div>`

let makeDefaultRootWithRenderedData = data => `<div id="app">${data}</div>`

let template = Fs.readFileSync(Path.join2(srcPath, "../demo/index.html"), "utf8")

let rendered = ReactDOMServer.renderToString(<Example />)

let updatedTemplate =
  template->String.replace(defaultRoot, makeDefaultRootWithRenderedData(rendered))

Fs.writeFileSync(Path.join2(srcPath, "../demo/index-static.html"), updatedTemplate)

Js.log(updatedTemplate)

Js.log("Success")
