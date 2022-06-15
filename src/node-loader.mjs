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

const isBsArtifact = url => {
  return url.match(/file:.*\.bs\.js/i)
}

// TODO Detect not only jpeg but any other type
const isJpeg = url => {
  return url.match(/file:.*\.jpg|jpeg/i)
}

export async function load(url, context, defaultLoad) {
  if (isBsArtifact(url)) {
    const format = "module"
    return defaultLoad(url, { format })
  } else if (isJpeg(url)) {
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
