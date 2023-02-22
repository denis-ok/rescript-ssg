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

let unsafeStringifyPropValue = data => {
  // We need a way to take a prop value of any type and inject it to generated React app template.
  // This is unsafe. Prop value should contain only values that is possible to JSON.stringify<->JSON.parse.
  // So it should be composed only of simple values. Types like functions, dates, promises etc can't be stringified.
  Jsesc.jsesc(
    data,
  );
};

let makeStringToImportJsFileFromRescript =
    (
      ~pageDataType: PageData.t,
      ~jsDataFilename: string,
      ~relativePathToDataDir: string,
    ) => {
  let valueName = PageData.toValueName(pageDataType);
  {j|
type $(valueName)
@module("$(relativePathToDataDir)/$(jsDataFilename)") external $(valueName): $(valueName) = "data"|j};
};

let renderReactAppTemplate =
    (
      ~importPageWrapperDataString="",
      ~importPageDataString="",
      elementString: string,
    ) => {
  {j|
$(importPageWrapperDataString)
$(importPageDataString)

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
  let script = helmet.script.toString();
  let noscript = helmet.noscript.toString();
  let style = helmet.style.toString();
  let bodyAttributes = helmet.bodyAttributes.toString();

  {j|<!DOCTYPE html>
<html $(htmlAttributes)>
  <head>
    <meta charset="utf-8"/>
    $(title)
    $(meta)
    $(link)
    $(script)
    $(noscript)
    $(style)
    $(headCssStyleTag)
    $(emotionStyleTag)
  </head>
  <body $(bodyAttributes)>
    <div id="root">$(renderedHtml)</div>
  </body>
</html>
|j};
};

type processedDataProp = {
  rescriptImportString: string,
  jsDataFileContent: string,
  jsDataFilepath: string,
};

type processedPage = {
  element: React.element,
  elementString: string,
  pageDataProp: option(processedDataProp),
  pageWrapperDataProp: option(processedDataProp),
};

let makeDataPropString = (pageDataType: PageData.t) => {
  let dataValueName = PageData.toValueName(pageDataType);
  {j|{$(dataValueName)->Obj.magic}|j};
};

let pageWrappersDataDirname = "__pageWrappersData";

let makeProcessedDataProp =
    (
      ~data: 'a,
      ~pageDataType: PageData.t,
      ~moduleName: string,
      ~pageOutputDir,
      ~pageWrappersDataDir,
    )
    : processedDataProp => {
  let relativePathToDataDir =
    switch (pageDataType) {
    | PageData => "."
    | PageWrapperData =>
      let relativePath =
        Path.relative(~from=pageOutputDir, ~to_=pageWrappersDataDir);
      if (relativePath->Js.String2.startsWith(pageWrappersDataDirname)) {
        "./" ++ relativePath;
      } else {
        relativePath;
      };
    };

  let stringifiedData = unsafeStringifyPropValue(data);

  let propDataHash = Crypto.Hash.stringToHash(stringifiedData);

  let jsDataFilename = moduleName ++ "_Data_" ++ propDataHash ++ ".js";

  let rescriptImportString =
    makeStringToImportJsFileFromRescript(
      ~pageDataType,
      ~relativePathToDataDir,
      ~jsDataFilename,
    );

  let jsDataFileContent = {j|export const data = $(stringifiedData)|j};

  let jsDataFilepath =
    switch (pageDataType) {
    | PageData => Path.join2(pageOutputDir, jsDataFilename)
    | PageWrapperData => Path.join2(pageWrappersDataDir, jsDataFilename)
    };

  {rescriptImportString, jsDataFileContent, jsDataFilepath};
};

let processPageComponentWithWrapper =
    (
      ~pageComponent: component,
      ~pageWrapper: option(pageWrapper),
      ~pageModuleName: string,
      ~pageOutputDir: string,
      ~pageWrappersDataDir: string,
    )
    : processedPage => {
  let {element, elementString, pageDataProp, _} =
    switch (pageComponent) {
    | ComponentWithoutData(element) => {
        element,
        elementString: "<" ++ pageModuleName ++ " />",
        pageDataProp: None,
        pageWrapperDataProp: None,
      }
    | ComponentWithData({component, data}) =>
      let pageDataType = PageData.PageData;
      let dataPropString = makeDataPropString(pageDataType);
      let elementString =
        "<"
        ++ pageModuleName
        ++ " "
        ++ dataPropName
        ++ "="
        ++ dataPropString
        ++ " />";

      let element = component(data);

      let pageDataProp =
        makeProcessedDataProp(
          ~pageDataType,
          ~data,
          ~moduleName=pageModuleName,
          ~pageOutputDir,
          ~pageWrappersDataDir,
        );

      {
        element,
        elementString,
        pageDataProp: Some(pageDataProp),
        pageWrapperDataProp: None,
      };
    };

  switch (pageWrapper) {
  | None => {element, elementString, pageDataProp, pageWrapperDataProp: None}
  | Some({component, modulePath}) =>
    let wrapperModuleName = Utils.getModuleNameFromModulePath(modulePath);
    switch (component) {
    | WrapperWithChildren(f) =>
      let wrapperOpenTag = "<" ++ wrapperModuleName ++ ">";
      let wrapperCloseTag = "</" ++ wrapperModuleName ++ ">";
      let wrappedElementString =
        wrapperOpenTag ++ elementString ++ wrapperCloseTag;

      let wrappedElement = f(element);

      {
        element: wrappedElement,
        elementString: wrappedElementString,
        pageDataProp,
        pageWrapperDataProp: None,
      };
    | WrapperWithDataAndChildren({component, data}) =>
      let pageDataType = PageData.PageWrapperData;
      let dataPropString = makeDataPropString(pageDataType);
      let wrapperOpenTag =
        "<"
        ++ wrapperModuleName
        ++ " "
        ++ dataPropName
        ++ "="
        ++ dataPropString
        ++ " >";
      let wrapperCloseTag = "</" ++ wrapperModuleName ++ ">";
      let wrappedElementString =
        wrapperOpenTag ++ elementString ++ wrapperCloseTag;

      let wrappedElement = component(data, element);

      let pageWrapperDataProp =
        makeProcessedDataProp(
          ~pageDataType,
          ~data,
          ~moduleName=wrapperModuleName,
          ~pageOutputDir,
          ~pageWrappersDataDir,
        );

      {
        element: wrappedElement,
        elementString: wrappedElementString,
        pageDataProp,
        pageWrapperDataProp: Some(pageWrapperDataProp),
      };
    };
  };
};

let buildPageHtmlAndReactApp = (~outputDir, ~logger: Log.logger, page: page) => {
  let intermediateFilesOutputDir = getIntermediateFilesOutputDir(~outputDir);

  let moduleName: string = Utils.getModuleNameFromModulePath(page.modulePath);

  let pagePath: string = page.path->PageBuilderT.PagePath.toString;

  let pageOutputDir = Path.join2(intermediateFilesOutputDir, pagePath);

  let pageWrappersDataDir =
    Path.join2(intermediateFilesOutputDir, pageWrappersDataDirname);

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

  let () = Fs.mkDirSync(pageWrappersDataDir, {recursive: true});

  let {element, elementString, pageDataProp, pageWrapperDataProp} =
    processPageComponentWithWrapper(
      ~pageComponent=page.component,
      ~pageWrapper=page.pageWrapper,
      ~pageModuleName=moduleName,
      ~pageOutputDir,
      ~pageWrappersDataDir,
    );

  let resultHtml =
    renderHtmlTemplate(
      ~pageElement=element,
      ~headCssFilepaths=page.headCssFilepaths,
    );

  let resultReactApp =
    renderReactAppTemplate(
      ~importPageWrapperDataString=?
        Belt.Option.map(pageWrapperDataProp, v => v.rescriptImportString),
      ~importPageDataString=?
        Belt.Option.map(pageDataProp, v => v.rescriptImportString),
      elementString,
    );

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
    [|pageWrapperDataProp, pageDataProp|]
    ->Js.Array2.forEach(data =>
        switch (data) {
        | None => ()
        | Some({jsDataFileContent, jsDataFilepath, _}) =>
          Fs.writeFileSync(jsDataFilepath, jsDataFileContent)
        }
      );

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
