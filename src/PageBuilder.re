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

module PageData = {
  type t =
    | PageWrapperData
    | PageData;

  let toValueName = (t: t) =>
    switch (t) {
    | PageWrapperData => "pageWrapperData"
    | PageData => "pageData"
    };
};

let unsafeStringifyPropValue' = data =>
  // We need a way to take a prop value of any type and inject it to generated React app template.
  // We take a prop and inject it's JSON stringified->parsed value in combination with Obj.magic.
  // This is unsafe. Prop value should contain only values that possible to JSON.stringify<->JSON.parse.
  // So it should be composed only of simple values. Types like functions, dates, promises etc can't be stringified.
  switch (data->Js.Json.stringifyAny) {
  | Some(propValueString) => propValueString
  | None =>
    // Js.Json.stringifyAny(None) returns None. No need to do anything with it, can be injected to template as is.
    "None"
  };

let makeJsDataModuleName = (~moduleName) => moduleName ++ "Data";

let makeJsDataFileTemplate = data => {
  let data = unsafeStringifyPropValue'(data);
  {j|export const data = `$(data)`|j};
};

let makeImportLine =
    (
      ~pageDataType: PageData.t,
      ~moduleName: string,
      ~pathToPageDataDir: string,
    ) => {
  let valueName = PageData.toValueName(pageDataType);
  let moduleName = makeJsDataModuleName(~moduleName);
  // TODO If we write page's data (not page wrapper data),
  //  we should put it to page's dir
  {j|@module("$(pathToPageDataDir)/$(moduleName).js") external $(valueName): string = "data";|j};
};

let renderReactAppTemplate =
    (~importPageWrapperDataString: option(string), elementString: string) => {
  let importPageWrapperDataString =
    importPageWrapperDataString->Belt.Option.getWithDefault("");

  {j|
$(importPageWrapperDataString)

switch ReactDOM.querySelector("#root") {
| Some(root) => ReactDOM.hydrate($(elementString), root)
| None => ()
}
|j};
};

let dataPropName = "data";

let getIntermediateFilesOutputDir = (~outputDir) =>
  Path.join2(outputDir, "temp");

let makeReactAppModuleName = (~pagePath, ~moduleName) => {
  let modulePrefix =
    pagePath
    ->Js.String2.replaceByRe([%re {|/\//g|}], "")
    ->Js.String2.replaceByRe([%re {|/-/g|}], "")
    ->Js.String2.replaceByRe([%re {|/\./g|}], "");

  modulePrefix ++ moduleName ++ "App";
};

let renderHtmlTemplate =
    (~pageElement: React.element, ~headCssFilepaths: array(string)): string => {
  let html = ReactDOMServer.renderToString(pageElement);

  let {html: renderedHtml, css, ids} = Emotion.Server.extractCritical(html);

  let emotionIds = ids->Js.Array2.joinWith(" ");

  // https://github.com/emotion-js/emotion/blob/92be52d894c7d81d013285e9dfe90820e6b178f8/packages/css/src/index.js#L15
  let emotionCacheKey = "css";

  let emotionStyleTag = {j|<style data-emotion="$(emotionCacheKey) $(emotionIds)">$(css)</style>|j};

  let headCss =
    switch (headCssFilepaths) {
    | [||] => None
    | cssFiles =>
      Some(
        cssFiles
        ->Js.Array2.map(filepath => Fs.readFileSyncAsUtf8(filepath))
        ->Js.Array2.joinWith("\n"),
      )
    };

  let headCssStyleTag =
    switch (headCss) {
    | None => ""
    | Some(css) => "<style>" ++ css ++ "</style>"
    };

  let helmet = ReactHelmet.renderStatic();

  let htmlAttributes = helmet.htmlAttributes.toString();
  let title = helmet.title.toString();
  let meta = helmet.meta.toString();
  let link = helmet.link.toString();
  let bodyAttributes = helmet.bodyAttributes.toString();

  {j|<!DOCTYPE html>
<html $(htmlAttributes)>
  <head>
    <meta charset="utf-8"/>
    $(title)
    $(meta)
    $(link)
    $(headCssStyleTag)
    $(emotionStyleTag)
  </head>
  <body $(bodyAttributes)>
    <div id="root">$(renderedHtml)</div>
  </body>
</html>
|j};
};

let unsafeStringifyPropValue = data =>
  // We need a way to take a prop value of any type and inject it to generated React app template.
  // We take a prop and inject it's JSON stringified->parsed value in combination with Obj.magic.
  // This is unsafe. Prop value should contain only values that possible to JSON.stringify<->JSON.parse.
  // So it should be composed only of simple values. Types like functions, dates, promises etc can't be stringified.
  switch (data->Js.Json.stringifyAny) {
  | Some(propValueString) => {j|{`$(propValueString)`->Js.Json.parseExn->Obj.magic}|j}
  | None =>
    // Js.Json.stringifyAny(None) returns None. No need to do anything with it, can be injected to template as is.
    "None"
  };

let processPageComponentWithWrapper =
    (
      ~pageComponent: component,
      ~pageWrapper: option(pageWrapper),
      ~moduleName: string,
    )
    : (React.element, string) => {
  let (element, elementString) =
    switch (pageComponent) {
    | ComponentWithoutData(element) => (element, "<" ++ moduleName ++ " />")
    | ComponentWithData({component, data}) =>
      let unsafeStringifiedPropValue = unsafeStringifyPropValue(data);

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

  switch (pageWrapper) {
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

      let dataValueName = PageData.toValueName(PageWrapperData);

      let dataPropString = {j|{$(dataValueName)->Js.Json.parseExn->Obj.magic}|j};

      let wrapperOpenTag =
        "<"
        ++ moduleName
        ++ " "
        ++ dataPropName
        ++ "="
        ++ dataPropString
        ++ " >";

      let wrapperCloseTag = "</" ++ moduleName ++ ">";

      let elementString = wrapperOpenTag ++ elementString ++ wrapperCloseTag;

      (wrappedElement, elementString);
    };
  };
};

let buildPageHtmlAndReactApp = (~outputDir, ~logger: Log.logger, page: page) => {
  let intermediateFilesOutputDir = getIntermediateFilesOutputDir(~outputDir);

  let moduleName: string = Utils.getModuleNameFromModulePath(page.modulePath);

  let pagePath: string = page.path->PageBuilderT.PagePath.toString;

  let pageOutputDir = Path.join2(intermediateFilesOutputDir, pagePath);

  logger.info(() =>
    Js.log(
      {j|[PageBuilder.buildPageHtmlAndReactApp] Building page module: $(moduleName), page path: $(pagePath)|j},
    )
  );

  logger.debug(() =>
    Js.log2(
      "[PageBuilder.buildPageHtmlAndReactApp] Output dir for page: ",
      pageOutputDir,
    )
  );

  let () = Fs.mkDirSync(pageOutputDir, {recursive: true});

  let (element, elementString) =
    processPageComponentWithWrapper(
      ~pageComponent=page.component,
      ~pageWrapper=page.pageWrapper,
      ~moduleName,
    );

  let resultHtml =
    renderHtmlTemplate(
      ~pageElement=element,
      ~headCssFilepaths=page.headCssFilepaths,
    );

  let pageWrappersDataDir =
    Path.join2(intermediateFilesOutputDir, "__pageWrappersData");

  let () = Fs.mkDirSync(pageWrappersDataDir, {recursive: true});

  let relativePathToPageWrappersDataDir =
    Path.relative(~from=pageOutputDir, ~to_=pageWrappersDataDir);

  let (importPageWrapperDataString, wrapperDataFileContent, wrapperModuleName) =
    switch (page.pageWrapper) {
    | Some({component: WrapperWithDataAndChildren({data, _}), modulePath}) =>
      let moduleName = Utils.getModuleNameFromModulePath(modulePath);
      let importLine =
        makeImportLine(
          ~pageDataType=PageWrapperData,
          ~pathToPageDataDir=relativePathToPageWrappersDataDir,
          ~moduleName,
        );
      let dataModuleContent = makeJsDataFileTemplate(data);
      (Some(importLine), Some(dataModuleContent), Some(moduleName));
    | Some(_)
    | None => (None, None, None)
    };

  let resultReactApp =
    renderReactAppTemplate(~importPageWrapperDataString, elementString);

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

  let () =
    // Here we write page wrapper's data to a separate JS module.
    switch (wrapperModuleName, wrapperDataFileContent) {
    | (Some(wrapperModuleName), Some(wrapperDataFileContent)) =>
      let pageWrapperDataFilename =
        makeJsDataModuleName(~moduleName=wrapperModuleName) ++ ".js";

      Fs.writeFileSync(
        Path.join2(pageWrappersDataDir, pageWrapperDataFilename),
        wrapperDataFileContent,
      );
    | _ => ()
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
  let durationLabel = "[PageBuilder.buildPages] duration";
  Js.Console.timeStart(durationLabel);

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

  logger.info(() => {
    Js.log("[PageBuilder.buildPages] Pages build finished successfully!");
    Js.Console.timeEnd(durationLabel);
  });

  ();
};
