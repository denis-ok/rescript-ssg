type pageAppArtifactsType =
  | Reason
  | Js;

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
  path: PagePath.t,
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

let getArtifactsOutputDir = (~outputDir) =>
  Path.join2(outputDir, "artifacts");

let pagePathToPageAppModuleName =
    (~pageAppArtifactsSuffix, ~pagePath, ~moduleName) => {
  let modulePrefix =
    pagePath
    ->Js.String2.replaceByRe([%re {|/\//g|}], "")
    ->Js.String2.replaceByRe([%re {|/-/g|}], "")
    ->Js.String2.replaceByRe([%re {|/\./g|}], "");

  modulePrefix ++ moduleName ++ "__PageApp" ++ pageAppArtifactsSuffix;
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

  let html = ReactDOM.Server.renderToString(pageElement);

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
  jsDataFileContent: string,
  jsDataFilepath: string,
};

let pageWrappersDataDirname = "__pageWrappersData";

module ReasonArtifact = {
  let renderReactAppTemplate =
      (
        ~importPageWrapperDataString="",
        ~importPageDataString="",
        elementString: string,
      ) => {
    {j|
$(importPageWrapperDataString)
$(importPageDataString)

switch (ReactDOM.querySelector("#root")) {
| Some(root) => ReactDOM.hydrate($(elementString), root)
| None => ()
};
|j};
  };

  type processedDataPropReason = {
    processedDataProp,
    importString: string,
  };

  type processedPage = {
    element: React.element,
    elementString: string,
    pageDataProp: option(processedDataPropReason),
    pageWrapperDataProp: option(processedDataPropReason),
  };

  let makeStringToImportJsFileFromReason =
      (
        ~pageDataType: PageData.t,
        ~jsDataFilename: string,
        ~relativePathToDataDir: string,
      ) => {
    let valueName = PageData.toValueName(pageDataType);
    {j|
type $(valueName);
[@mel.module "$(relativePathToDataDir)/$(jsDataFilename)"] external $(valueName): $(valueName) = "data";|j};
  };

  let makeDataPropString = (pageDataType: PageData.t) => {
    let dataValueName = PageData.toValueName(pageDataType);
    {j|{$(dataValueName)->Obj.magic}|j};
  };

  let makeProcessedDataProp =
      (
        ~data: 'a,
        ~pageDataType: PageData.t,
        ~moduleName: string,
        ~pageOutputDir: string,
        ~melangePageOutputDir: option(string),
        ~pageWrappersDataDir,
      )
      : processedDataPropReason => {
    let relativePathToDataDir =
      switch (pageDataType) {
      | PageData =>
        switch (melangePageOutputDir) {
        | None => "."
        | Some(melangePageOutputDir) =>
          let relativePath =
            Path.relative(~from=melangePageOutputDir, ~to_=pageOutputDir);
          relativePath;
        }

      | PageWrapperData =>
        let from =
          switch (melangePageOutputDir) {
          | None => pageOutputDir
          | Some(melangePageOutputDir) => melangePageOutputDir
          };
        let relativePath = Path.relative(~from, ~to_=pageWrappersDataDir);
        if (relativePath->Js.String2.startsWith(pageWrappersDataDirname)) {
          "./" ++ relativePath;
        } else {
          relativePath;
        };
      };

    let stringifiedData = unsafeStringifyPropValue(data);

    let propDataHash = Crypto.Hash.stringToHash(stringifiedData);

    let jsDataFilename = moduleName ++ "_Data_" ++ propDataHash ++ ".mjs";

    let importString =
      makeStringToImportJsFileFromReason(
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

    {
      processedDataProp: {
        jsDataFileContent,
        jsDataFilepath,
      },
      importString,
    };
  };

  let dataPropName = "data";

  let processPageComponentWithWrapper =
      (
        ~pageComponent: component,
        ~pageWrapper: option(pageWrapper),
        ~pageModuleName: string,
        ~pageOutputDir: string,
        ~melangePageOutputDir: option(string),
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
            ~melangePageOutputDir,
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
    | None => {
        element,
        elementString,
        pageDataProp,
        pageWrapperDataProp: None,
      }
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
            ~melangePageOutputDir,
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
};

module JsArtifact = {
  let renderElementTemplate =
      (
        ~componentName,
        ~dataProp: option(string),
        ~childrenProp: option(string),
      ) => {
    let dataPropString: string =
      switch (dataProp) {
      | None => "undefined"
      | Some(dataProp) => dataProp
      };
    let childrenPropString: string =
      switch (childrenProp) {
      | None => "undefined"
      | Some(childrenProp) => childrenProp
      };
    {j|
React.createElement($(componentName).make, {
  data: $(dataPropString),
  children: $(childrenPropString),
})
|j};
  };

  let renderReactAppTemplate =
      (
        ~pageArtifactPath: string,
        ~pageWrapperArtifactPath: option(string),
        ~pageDataPath: option(string),
        ~pageWrapperDataPath: option(string),
      ) => {
    let (pageDataImport, pageDataProp) =
      switch (pageDataPath) {
      | None => ("", None)
      | Some(pageDataPath) =>
        let dataPropImport = {j|import {data as pageData} from "$(pageDataPath)";|j};
        let dataValueName = "pageData";
        (dataPropImport, Some(dataValueName));
      };

    let pageElement =
      renderElementTemplate(
        ~componentName="Page",
        ~dataProp=pageDataProp,
        ~childrenProp=None,
      );

    let (pageWrapperImport, pageWrapperDataImport, elementString) =
      switch (pageWrapperArtifactPath) {
      | None => ("", "", pageElement)
      | Some(pageWrapperArtifactPath) =>
        let pageWrapperImport = {j|import * as PageWrapper from "$(pageWrapperArtifactPath)";|j};
        switch (pageWrapperDataPath) {
        | None =>
          let pageWrapperElement =
            renderElementTemplate(
              ~componentName="PageWrapper",
              ~dataProp=None,
              ~childrenProp=Some(pageElement),
            );
          (pageWrapperImport, "", pageWrapperElement);
        | Some(pageWrapperDataPath) =>
          let dataPropImport = {j|import {data as pageWrapperData} from "$(pageWrapperDataPath)";|j};
          let dataValueName = "pageWrapperData";
          let pageWrapperElement =
            renderElementTemplate(
              ~componentName="PageWrapper",
              ~dataProp=Some(dataValueName),
              ~childrenProp=Some(pageElement),
            );
          (pageWrapperImport, dataPropImport, pageWrapperElement);
        };
      };

    {j|
import * as React from "react";
import * as ReactDom from "react-dom";
import * as Page from "$(pageArtifactPath)";
$(pageDataImport)
$(pageWrapperImport)
$(pageWrapperDataImport)

const root = document.querySelector("#root");

if (root !== null) {
  ReactDom.hydrate($(elementString), root);
}
|j};
  };

  type processedPage = {
    element: React.element,
    pageDataProp: option(processedDataProp),
    pageWrapperDataProp: option(processedDataProp),
  };

  let makeProcessedDataProp =
      (
        ~data: 'a,
        ~pageDataType: PageData.t,
        ~moduleName: string,
        ~pageOutputDir: string,
        ~pageWrappersDataDir,
      )
      : processedDataProp => {
    let stringifiedData = unsafeStringifyPropValue(data);

    let propDataHash = Crypto.Hash.stringToHash(stringifiedData);

    let jsDataFilename = moduleName ++ "_Data_" ++ propDataHash ++ ".mjs";

    let jsDataFileContent = {j|export const data = $(stringifiedData)|j};

    let jsDataFilepath =
      switch (pageDataType) {
      | PageData => Path.join2(pageOutputDir, jsDataFilename)
      | PageWrapperData => Path.join2(pageWrappersDataDir, jsDataFilename)
      };

    {jsDataFileContent, jsDataFilepath};
  };

  let processPageComponentWithWrapperJs =
      (
        ~pageComponent: component,
        ~pageWrapper: option(pageWrapper),
        ~pageModuleName: string,
        ~pageOutputDir: string,
        ~pageWrappersDataDir: string,
      )
      : processedPage => {
    let {element, pageDataProp, _} =
      switch (pageComponent) {
      | ComponentWithoutData(element) => {
          element,
          pageDataProp: None,
          pageWrapperDataProp: None,
        }
      | ComponentWithData({component, data}) =>
        let pageDataType = PageData.PageData;
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
          pageDataProp: Some(pageDataProp),
          pageWrapperDataProp: None,
        };
      };

    switch (pageWrapper) {
    | None => {element, pageDataProp, pageWrapperDataProp: None}
    | Some({component, modulePath}) =>
      let wrapperModuleName = Utils.getModuleNameFromModulePath(modulePath);
      switch (component) {
      | WrapperWithChildren(f) =>
        let wrappedElement = f(element);
        {element: wrappedElement, pageDataProp, pageWrapperDataProp: None};
      | WrapperWithDataAndChildren({component, data}) =>
        let pageDataType = PageData.PageWrapperData;
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
          pageDataProp,
          pageWrapperDataProp: Some(pageWrapperDataProp),
        };
      };
    };
  };
};

let buildPageHtmlAndReactApp =
    (
      ~pageAppArtifactsType: pageAppArtifactsType,
      ~outputDir: string,
      ~melangeOutputDir: option(string),
      ~logger: Log.logger,
      ~pageAppArtifactsSuffix: string,
      page: page,
    ) => {
  let artifactsOutputDir = getArtifactsOutputDir(~outputDir);

  let moduleName: string = Utils.getModuleNameFromModulePath(page.modulePath);

  let pagePath: string = page.path->PagePath.toString;

  let pageOutputDir = Path.join2(artifactsOutputDir, pagePath);

  // Melange emits compiled JS files to a separate dir (not next to Reason files).
  // We need to handle it to build correct relative paths to entry files and to prop data files.
  let melangePageOutputDir =
    switch (melangeOutputDir) {
    | None => None
    | Some(melangeOutputDir) =>
      let melangeArtifactsOutputDir =
        getArtifactsOutputDir(~outputDir=melangeOutputDir);
      Some(Path.join2(melangeArtifactsOutputDir, pagePath));
    };

  let pageWrappersDataDir =
    Path.join2(artifactsOutputDir, pageWrappersDataDirname);

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

  let modulesWithHydration__Mutable = [||];

  let (resultHtml, resultReactApp, pageDataProp, pageWrapperDataProp) =
    switch (pageAppArtifactsType) {
    | Reason =>
      let {
        ReasonArtifact.element,
        elementString,
        pageDataProp,
        pageWrapperDataProp,
      } =
        ReasonArtifact.processPageComponentWithWrapper(
          ~pageComponent=page.component,
          ~pageWrapper=page.pageWrapper,
          ~pageModuleName=moduleName,
          ~pageOutputDir,
          ~melangePageOutputDir,
          ~pageWrappersDataDir,
        );
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
          ReasonArtifact.renderReactAppTemplate(
            ~importPageWrapperDataString=?
              Belt.Option.map(pageWrapperDataProp, v => v.importString),
            ~importPageDataString=?
              Belt.Option.map(pageDataProp, v => v.importString),
            elementString,
          )
        | PartialHydration =>
          PartialHydration.renderReactAppTemplate(
            ~modulesWithHydration__Mutable,
          )
        };
      (
        resultHtml,
        resultReactApp,
        pageDataProp->Belt.Option.map(v => v.processedDataProp),
        pageWrapperDataProp->Belt.Option.map(v => v.processedDataProp),
      );
    | Js =>
      let {JsArtifact.element, pageDataProp, pageWrapperDataProp} =
        JsArtifact.processPageComponentWithWrapperJs(
          ~pageComponent=page.component,
          ~pageWrapper=page.pageWrapper,
          ~pageModuleName=moduleName,
          ~pageOutputDir,
          ~pageWrappersDataDir,
        );
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
        JsArtifact.renderReactAppTemplate(
          ~pageArtifactPath=page.modulePath,
          ~pageWrapperArtifactPath={
            switch (page.pageWrapper) {
            | None => None
            | Some(wrapper) => Some(wrapper.modulePath)
            };
          },
          ~pageDataPath={
            switch (pageDataProp) {
            | None => None
            | Some({jsDataFilepath, _}) => Some(jsDataFilepath)
            };
          },
          ~pageWrapperDataPath={
            switch (pageWrapperDataProp) {
            | None => None
            | Some({jsDataFilepath, _}) => Some(jsDataFilepath)
            };
          },
        );
      (resultHtml, resultReactApp, pageDataProp, pageWrapperDataProp);
    };

  let pageAppModuleName =
    pagePathToPageAppModuleName(
      ~pageAppArtifactsSuffix,
      ~pagePath,
      ~moduleName,
    );

  let resultHtmlPath = Path.join2(pageOutputDir, "index.html");

  let mkDirPromises =
    [|
      Fs.Promises.mkDir(pageOutputDir, {recursive: true})
      ->Promise.Result.catch(
          ~context=
            "[PageBuilder.buildPageHtmlAndReactApp] [Fs.Promises.mkDir(pageOutputDir)]",
        ),
      Fs.Promises.mkDir(pageWrappersDataDir, {recursive: true})
      ->Promise.Result.catch(
          ~context=
            "[PageBuilder.buildPageHtmlAndReactApp] [Fs.Promises.mkDir(pageWrappersDataDir)]",
        ),
    |]
    ->Promise.all
    ->Promise.Result.all;

  let writeFilePromises =
    mkDirPromises->Promise.Result.flatMap(_createdDirs => {
      let pageAppModuleExtension =
        switch (pageAppArtifactsType) {
        | Reason => ".re"
        | Js => ".mjs"
        };

      let reactAppFilename = pageAppModuleName ++ pageAppModuleExtension;

      let resultHtmlFilePromise =
        Fs.Promises.writeFile(~path=resultHtmlPath, ~data=resultHtml)
        ->Promise.Result.catch(
            ~context=
              "[PageBuilder.buildPageHtmlAndReactApp] [resultHtmlFilePromise]",
          );

      let resultReactAppFilePromise =
        Fs.Promises.writeFile(
          ~path=Path.join2(pageOutputDir, reactAppFilename),
          ~data=resultReactApp,
        )
        ->Promise.Result.catch(
            ~context=
              "[PageBuilder.buildPageHtmlAndReactApp] [resultReactAppFilePromise]",
          );

      let jsFilesPromises =
        [|pageWrapperDataProp, pageDataProp|]
        ->Js.Array2.map(data =>
            switch (data) {
            | None => Promise.resolve(Belt.Result.Ok())
            | Some({jsDataFileContent, jsDataFilepath, _}) =>
              Fs.Promises.writeFile(
                ~path=jsDataFilepath,
                ~data=jsDataFileContent,
              )
              ->Promise.Result.catch(
                  ~context=
                    "[PageBuilder.buildPageHtmlAndReactApp] [jsFilesPromises]",
                )
            }
          );

      let promises =
        Js.Array2.concat(
          [|resultHtmlFilePromise, resultReactAppFilePromise|],
          jsFilesPromises,
        );

      promises->Promise.all->Promise.Result.all;
    });

  writeFilePromises->Promise.Result.map(_createdFiles => {
    let compiledReactAppFilename =
      switch (pageAppArtifactsType) {
      | Reason => ".bs.js"
      | Js => ".mjs"
      };

    let compiledReactAppFilename =
      pageAppModuleName ++ compiledReactAppFilename;

    let renderedPage: RenderedPage.t = {
      path: page.path,
      entryPath: {
        switch (pageAppArtifactsType) {
        | Js => Path.join2(pageOutputDir, compiledReactAppFilename)
        | Reason =>
          Path.join2(
            melangePageOutputDir->Belt.Option.getWithDefault(pageOutputDir),
            compiledReactAppFilename,
          )
        };
      },
      outputDir: pageOutputDir,
      htmlTemplatePath: resultHtmlPath,
    };

    logger.debug(() =>
      Js.log2(
        "[PageBuilder.buildPageHtmlAndReactApp] Build finished: ",
        moduleName,
      )
    );

    renderedPage;
  });
};
