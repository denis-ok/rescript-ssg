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
