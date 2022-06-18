// We need to fix the following error that appeared after bs-css added:
// /Users/denis/projects/builder/node_modules/bs-css-emotion/src/Css.bs.js:3
// import * as Curry from "rescript/lib/es6/curry.js";
// ^^^^^^

// The "load" function defined in this file forces NodeJS to load bs-artifacts as es6 module
// https://nodejs.org/api/esm.html#loaders

import path from "path"
import * as Webpack from "./Webpack.bs.js"
import fs from 'fs'
// import { dirname } from 'path';
// import { fileURLToPath } from 'url';
// const __dirname = dirname(fileURLToPath(import.meta.url));

const dataToHash = Webpack.Hash.dataToHash

const webpackAssetsDir = Webpack.webpackAssetsDir

const assetRegex = Webpack.assetRegex

const isBsArtifact = url => {
  return url.match(/file:.*\.bs\.js$/i)
}

// const isImage = url => {
//   return url.match(/file:.*\.(jpg|jpeg|png|gif|svg|ico|avif|webp)$/i)
// }

// const isFont = url => {
//   return url.match(/file:.*\.(woff|woff2)$/i)
// }

// const isJson = url => {
//   return url.match(/file:.*\.json$/i)
// }

const isAsset = url => url.match(assetRegex)

export async function load(url, context, defaultLoad) {
  if (isBsArtifact(url)) {
    const format = "module"
    return defaultLoad(url, { format })
  } else if (isAsset(url)) {
    // Any other file can't be loaded by Node as is so we need to handle them.
    // We get a hash from file's data and export path where this file will be located after webpack build.
    // TODO Move code below to Reason
    const filepath = url.replace("file://", "")
    const fileData = fs.readFileSync(filepath)
    const fileHash = dataToHash(fileData)
    const fileName = path.basename(url)
    const fileExt = path.extname(fileName)
    const filenameWithoutExt = fileName.replace(fileExt, "")
    const filenameWithHash = `${filenameWithoutExt}.${fileHash}${fileExt}`
    const webpackAssetPath = path.join(webpackAssetsDir, filenameWithHash)
    return Promise.resolve({
      source: `export default "${webpackAssetPath}";`,
      format: "module",
    })
  } else {
    // Defer to Node.js for all other URLs.
    return defaultLoad(url, context, defaultLoad)
  }
}
