import * as Webpack from "../Webpack.bs.js"
import * as NodeLoader from "../NodeLoader.bs.js"

const isBsArtifact = url => url.match(/file:.*\.bs\.js$/i)

const isAsset = url => url.match(Webpack.assetRegex)

// 'v16.15.0' => 16150
const nodeVersionToInt = s => {
  const refinedNodeVersion = s.replaceAll("v", "").replaceAll(".", "")
  return parseInt(refinedNodeVersion, 10)
}

const nodeVersion = nodeVersionToInt(process.version)

export async function load(url, context, nextLoad) {
  if (isBsArtifact(url)) {
    // We need to fix the error that appeared after bs-css added:
    // /Users/denis/projects/builder/node_modules/bs-css-emotion/src/Css.bs.js:3
    // import * as Curry from "rescript/lib/es6/curry.js";
    // ^^^^^^
    // SyntaxError: Cannot use import statement outside a module
    // We force NodeJS to load bs-artifacts as es6 modules
    const format = "module"
    return nextLoad(url, { format })
  } else if (isAsset(url)) {
    return NodeLoader.processAsset(url, undefined)
  } else {
    // Defer to Node.js for all other URLs.
    if (nodeVersion >= 16170) {
      return nextLoad(url)
    } else {
      return nextLoad(url, context, nextLoad)
    }
  }
}
