import Fs from "fs";
import React from "react";
import ReactDOMServer from "react-dom/server.js";
import * as App from "../demo/App.bs.js";

import { fileURLToPath } from "url";
import Path, { dirname } from "path";

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

const rendered = ReactDOMServer.renderToString(
  React.createElement(App.make, {})
);

const defaultRoot = `<div id="app"></div>`;

const makeDefaultRootWithRenderedData = (data) => `<div id="app">${data}</div>`;

const template = Fs.readFileSync(
  Path.join(__dirname, "../demo/index.html"),
  "utf8"
);

const updatedTemplate = template.replace(
  defaultRoot,
  makeDefaultRootWithRenderedData(rendered)
);

console.log({ rendered });
console.log({ updatedTemplate });

Fs.writeFileSync(
  Path.join(__dirname, "../demo/index-static.html"),
  updatedTemplate
);
