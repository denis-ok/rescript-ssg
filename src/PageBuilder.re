type componentWithData('a) = {
  component: 'a => React.element,
  data: 'a,
};

type component =
  | ComponentWithoutData(React.element)
  | ComponentWithData(componentWithData('a)): component;

type wrapperComponentWithData('data) = {
  component: ('data, React.element) => React.element,
  data: 'data,
};

type wrapperComponent =
  | WrapperWithChildren(React.element => React.element)
  | WrapperWithDataAndChildren(wrapperComponentWithData('a))
    : wrapperComponent;

type pageWrapper = {
  component: wrapperComponent,
  modulePath: string,
};

type page = {
  pageWrapper: option(pageWrapper),
  component,
  modulePath: string,
  path: PageBuilderT.PagePath.t,
  headCssFilepaths: array(string),
};

let makeHtmlTemplate =
    (
      ~headCss: option(string),
      helmet: ReactHelmet.helmetInstance,
      renderedHtml: string,
    ) => {
  let headCss =
    switch (headCss) {
    | None => ""
    | Some(css) => {j|<style>$(css)</style>|j}
    };
  let htmlAttributes = helmet.htmlAttributes.toString();
  let title = helmet.title.toString();
  let meta = helmet.meta.toString();
  let link = helmet.link.toString();
  let bodyAttributes = helmet.bodyAttributes.toString();
  {j|
<!DOCTYPE html>
<html $(htmlAttributes)>
  <head>
    <meta charset="utf-8"/>
    $(title)
    $(meta)
    $(link)
    $(headCss)
  </head>
  <body $(bodyAttributes)>
    <div id="root">$(renderedHtml)</div>
  </body>
</html>
|j};
};

let makeReactAppTemplate = (elementString: string) => {j|
switch ReactDOM.querySelector("#root") {
| Some(root) => ReactDOM.hydrate($(elementString), root)
| None => ()
}
|j};

let dataPropName = "data";

let makeReactAppModuleName = (~pagePath, ~moduleName) => {
  let modulePrefix =
    pagePath
    ->Js.String2.replaceByRe([%re {|/\//g|}], "")
    ->Js.String2.replaceByRe([%re {|/-/g|}], "")
    ->Js.String2.replaceByRe([%re {|/\./g|}], "");

  modulePrefix ++ moduleName ++ "App";
};

let buildPageHtmlAndReactApp = (~outputDir, ~logger: Log.logger, page: page) => {
  let moduleName = Utils.getModuleNameFromModulePath(page.modulePath);

  let pagePath = page.path->PageBuilderT.PagePath.toString;

  let pageOutputDir = Path.join2(outputDir, pagePath);

  logger.debug(() =>
    Js.log2(
      "[PageBuilder.buildPageHtmlAndReactApp] Output dir for page: ",
      pageOutputDir,
    )
  );

  let () = Fs.mkDirSync(pageOutputDir, {recursive: true});

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
        | Some(propValueString) => {j|{`$(propValueString)`->Js.Json.parseExn->Obj.magic}|j}
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
      | WrapperWithChildren(f) =>
        let wrapperOpenTag = "<" ++ moduleName ++ ">";
        let wrapperCloseTag = "</" ++ moduleName ++ ">";
        let elementString = wrapperOpenTag ++ elementString ++ wrapperCloseTag;
        let wrappedElement = f(element);
        (wrappedElement, elementString);
      | WrapperWithDataAndChildren({component, data}) =>
        let wrappedElement = component(data, element);

        let unsafeStringifiedPropValue =
          switch (data->Js.Json.stringifyAny) {
          | Some(propValueString) => {j|{`$(propValueString)`->Js.Json.parseExn->Obj.magic}|j}
          | None =>
            // Js.Json.stringifyAny(None) returns None. No need to do anything with it, can be injected to template as is.
            "None"
          };

        let wrapperOpenTag =
          "<" ++ moduleName ++ " data=" ++ unsafeStringifiedPropValue ++ " >";

        let wrapperCloseTag = "</" ++ moduleName ++ ">";

        let elementString = wrapperOpenTag ++ elementString ++ wrapperCloseTag;

        (wrappedElement, elementString);
      };
    };
  };

  let html = ReactDOMServer.renderToString(element);

  let htmlWithStyles = Emotion.Server.renderStylesToString(html);

  let helmet = ReactHelmet.renderStatic();

  let headCss =
    switch (page.headCssFilepaths) {
    | [||] => None
    | cssFiles =>
      Some(
        cssFiles
        ->Js.Array2.map(filepath => Fs.readFileSyncAsUtf8(filepath))
        ->Js.Array2.joinWith("\n"),
      )
    };

  let resultHtml = makeHtmlTemplate(~headCss, helmet, htmlWithStyles);

  let resultReactApp = makeReactAppTemplate(elementString);

  let pageAppModuleName = makeReactAppModuleName(~pagePath, ~moduleName);

  let resultHtmlPath = Path.join2(pageOutputDir, "index.html");

  let () = {
    let reactAppFilename = pageAppModuleName ++ ".res";
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

  logger.debug(() =>
    Js.log2(
      "[PageBuilder.buildPageHtmlAndReactApp] Build finished: ",
      moduleName,
    )
  );
};

let buildPages = (~outputDir, ~logger: Log.logger, pages: array(page)) => {
  logger.info(() => Js.log("[PageBuilder.buildPages] Building pages..."));

  let pagesDict = Js.Dict.empty();

  let () =
    pages->Js.Array2.forEach(page => {
      let pagePath = PageBuilderT.PagePath.toString(page.path);
      switch (pagesDict->Js.Dict.get(pagePath)) {
      | None => pagesDict->Js.Dict.set(pagePath, page)
      | Some(_) =>
        logger.info(() =>
          Js.Console.error3(
            "[PageBuilder.buildPages] List of pages contains pages with the same paths. Page with path: ",
            page.path,
            " has already been built.",
          )
        );

        Process.exit(1);
      };

      buildPageHtmlAndReactApp(~outputDir, ~logger, page);
    });

  ();
};
