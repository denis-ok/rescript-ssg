import * as NodeLoader from "../NodeLoader.bs.js";

export async function load(url, context, nextLoad) {
  if (NodeLoader.isBsArtifact(url)) {
    // We need to fix the error that appeared after bs-css added:
    // /Users/denis/projects/builder/node_modules/bs-css-emotion/src/Css.bs.js:3
    // import * as Curry from "rescript/lib/es6/curry.js";
    // ^^^^^^
    // SyntaxError: Cannot use import statement outside a module
    // We force NodeJS to load bs-artifacts as es6 modules
    const format = "module";
    return nextLoad(url, { format });
  } else if (NodeLoader.isAsset(url)) {
    return NodeLoader.processAsset(url);
  } else {
    return nextLoad(url);
  }
}
