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

type hydrationMode =
  | FullHydration
  | PartialHydration;

type pageWrapper = {
  component: wrapperComponent,
  modulePath: string,
};

type page = {
  hydrationMode,
  pageWrapper: option(pageWrapper),
  component,
  modulePath: string,
  path: PageBuilderT.PagePath.t,
  headCssFilepaths: array(string),
  globalValues: option(array((string, Js.Json.t))),
  headScripts: array(string),
  bodyScripts: array(string),
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

let wrapJsTextWithScriptTag = (jsText: string) => {j|<script>$(jsText)</script>|j};

let globalValuesToScriptTag =
    (globalValues: array((string, Js.Json.t))): string => {
  globalValues
  ->Js.Array2.map(((key, value)) => {
      let keyS: string =
        switch (Js.Json.stringifyAny(key)) {
        | Some(key) => key
        | None =>
          Js.Console.error2(
            "[globalValuesToScriptTag] Failed to stringify JSON (globalValues key). key:",
            key,
          );
          Process.exit(1);
        };
      let valueS: string =
        switch (Js.Json.stringifyAny(value)) {
        | Some(value) => value
        | None =>
          Js.Console.error4(
            "[globalValuesToScriptTag] Failed to stringify JSON (globalValues value). key:",
            key,
            "value:",
            value,
          );
          Process.exit(1);
        };
      {j|globalThis[$(keyS)] = $(valueS)|j};
    })
  ->Js.Array2.joinWith("\n")
  ->wrapJsTextWithScriptTag;
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

let pagePathToPageAppModuleName =
    (~generatedFilesSuffix, ~pagePath, ~moduleName) => {
  let modulePrefix =
    pagePath
    ->Js.String2.replaceByRe([%re {|/\//g|}], "")
    ->Js.String2.replaceByRe([%re {|/-/g|}], "")
    ->Js.String2.replaceByRe([%re {|/\./g|}], "");

  modulePrefix ++ moduleName ++ "__PageApp" ++ generatedFilesSuffix;
};

let groupScripts = scripts =>
  switch (scripts) {
  | [||] => ""
  | scripts => "<script>" ++ scripts->Js.Array2.joinWith("\n") ++ "</script>"
  };

let renderHtmlTemplate =
    (
      ~hydrationMode: hydrationMode,
      ~modulesWithHydration__Mutable: array(string),
      ~pageElement: React.element,
      ~headCssFilepaths: array(string),
      ~globalValues: array((string, Js.Json.t)),
      ~headScripts: array(string),
      ~bodyScripts: array(string),
    )
    : string => {
  let pageElement =
    switch (hydrationMode) {
    | FullHydration => pageElement
    | PartialHydration =>
      <PartialHydration.WithHydrationContext.Provider
        modulesWithHydration__Mutable>
        pageElement
      </PartialHydration.WithHydrationContext.Provider>
    };

  let html = ReactDOMServer.renderToString(pageElement);

  let Emotion.Server.{html: renderedHtml, css, ids} =
    Emotion.Server.extractCritical(html);

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
  let scriptTagWithGlobalValues: string =
    globalValuesToScriptTag(globalValues);
  let headScript: string = headScripts->groupScripts;
  let bodyScript: string = bodyScripts->groupScripts;

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
    $(scriptTagWithGlobalValues)
    $(headScript)
  </head>
  <body $(bodyAttributes)>
    $(bodyScript)
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

let buildPageHtmlAndReactApp =
    (
      ~outputDir: string,
      ~melangeOutputDir: option(string),
      ~logger: Log.logger,
      ~generatedFilesSuffix: string,
      page: page,
    ) => {
  let intermediateFilesOutputDir = getIntermediateFilesOutputDir(~outputDir);

  let moduleName: string = Utils.getModuleNameFromModulePath(page.modulePath);

  let pagePath: string = page.path->PageBuilderT.PagePath.toString;

  let pageOutputDir = Path.join2(intermediateFilesOutputDir, pagePath);

  // Melange emits compiled JS files to a separate dir (not next to Reason files).
  // We need to handle it to build correct relative paths to webpack entries and to prop data files.
  let melangePageOutputDir =
    switch (melangeOutputDir) {
    | None => None
    | Some(melangeOutputDir) =>
      let melangeIntermediateFilesOutputDir =
        getIntermediateFilesOutputDir(~outputDir=melangeOutputDir);
      Some(Path.join2(melangeIntermediateFilesOutputDir, pagePath));
    };

  let pageWrappersDataDir =
    Path.join2(intermediateFilesOutputDir, pageWrappersDataDirname);

  logger.debug(() =>
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
      ~pageOutputDir=
        melangePageOutputDir->Belt.Option.getWithDefault(pageOutputDir),
      ~pageWrappersDataDir,
    );

  let modulesWithHydration__Mutable = [||];

  let resultHtml: string =
    renderHtmlTemplate(
      ~hydrationMode=page.hydrationMode,
      ~modulesWithHydration__Mutable,
      ~pageElement=element,
      ~headCssFilepaths=page.headCssFilepaths,
      ~globalValues=Belt.Option.getWithDefault(page.globalValues, [||]),
      ~headScripts=page.headScripts,
      ~bodyScripts=page.bodyScripts,
    );

  let resultReactApp =
    switch (page.hydrationMode) {
    | FullHydration =>
      renderReactAppTemplate(
        ~importPageWrapperDataString=?
          Belt.Option.map(pageWrapperDataProp, v => v.rescriptImportString),
        ~importPageDataString=?
          Belt.Option.map(pageDataProp, v => v.rescriptImportString),
        elementString,
      )
    | PartialHydration =>
      PartialHydration.renderReactAppTemplate(~modulesWithHydration__Mutable)
    };

  let pageAppModuleName =
    pagePathToPageAppModuleName(
      ~generatedFilesSuffix,
      ~pagePath,
      ~moduleName,
    );

  let resultHtmlPath = Path.join2(pageOutputDir, "index.html");

  let () = {
    let reactAppFilename = pageAppModuleName ++ ".res";
    Fs.writeFileSync(~path=resultHtmlPath, ~data=resultHtml);
    Fs.writeFileSync(
      ~path=Path.join2(pageOutputDir, reactAppFilename),
      ~data=resultReactApp,
    );
  };

  let () =
    [|pageWrapperDataProp, pageDataProp|]
    ->Js.Array2.forEach(data =>
        switch (data) {
        | None => ()
        | Some({jsDataFileContent, jsDataFilepath, _}) =>
          Fs.writeFileSync(~path=jsDataFilepath, ~data=jsDataFileContent)
        }
      );

  logger.debug(() =>
    Js.log2(
      "[PageBuilder.buildPageHtmlAndReactApp] Build finished: ",
      moduleName,
    )
  );

  let compiledReactAppFilename = pageAppModuleName ++ ".bs.js";

  let webpackPage: RenderedPage.t = {
    path: page.path,
    entryPath:
      Path.join2(
        melangePageOutputDir->Belt.Option.getWithDefault(pageOutputDir),
        compiledReactAppFilename,
      ),
    outputDir: pageOutputDir,
    htmlTemplatePath: resultHtmlPath,
  };

  webpackPage;
};

let checkPageDuplicates = (pages: array(page)) => {
  let pagesDict = Js.Dict.empty();

  pages->Js.Array2.forEach(page => {
    let pagePath = PageBuilderT.PagePath.toString(page.path);
    switch (pagesDict->Js.Dict.get(pagePath)) {
    | None => pagesDict->Js.Dict.set(pagePath, page)
    | Some(_) =>
      Js.Console.error2(
        "[PageBuilder.buildPages] List of pages contains pages with the same paths. Duplicated page path: ",
        pagePath,
      );
      Process.exit(1);
    };
  });
};

let buildPages =
    (
      ~outputDir,
      ~melangeOutputDir: option(string),
      ~logger: Log.logger,
      pages: array(page),
    ) => {
  checkPageDuplicates(pages);

  let durationLabel = "[PageBuilder.buildPages] duration";
  Js.Console.timeStart(durationLabel);

  logger.info(() => Js.log("[PageBuilder.buildPages] Building pages..."));

  let renderedPages =
    pages->Js.Array2.map(page => {
      buildPageHtmlAndReactApp(~outputDir, ~melangeOutputDir, ~logger, page)
    });

  logger.info(() => {
    Js.log("[PageBuilder.buildPages] Pages build finished successfully!");
    Js.Console.timeEnd(durationLabel);
  });

  renderedPages;
};
