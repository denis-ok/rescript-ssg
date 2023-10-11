let renderReactAppTemplate = (~pageModuleArtifactPath: string) => {
  {j|
import * as React from "react";
import * as ReactDom from "react-dom";
import * as Page from "$(pageModuleArtifactPath)";

const root = document.querySelector("#root");

if (!(root == null)) {
  ReactDom.hydrate(React.createElement(Page.make, {}), root);
}
|j};
};
