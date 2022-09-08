import path from "path"
import fs from 'fs'
import * as Webpack from "./Webpack.bs.js"
import * as CliArgs from "./CliArgs.bs.js"

const isBsArtifact = url => url.match(/file:.*\.bs\.js$/i)

const isAsset = url => url.match(Webpack.assetRegex)

const dataToHash = Webpack.Hash.dataToHash

export async function load(url, _context, nextLoad) {
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
    // Any other file can't be loaded by Node as is so we need to handle them.
    // We get a hash from file's data and export path where this file will be located after webpack build.
    // TODO Can we move the code below to Reason?
    const filepath = url.replace("file://", "")
    const fileData = fs.readFileSync(filepath)
    const fileHash = dataToHash(fileData)
    const fileName = path.basename(url)
    const fileExt = path.extname(fileName)
    const filenameWithoutExt = fileName.replace(fileExt, "")
    const filenameWithHash = `${filenameWithoutExt}.${fileHash}${fileExt}`
    const webpackAssetPath = path.join(CliArgs.assetPrefix, Webpack.webpackAssetsDir, filenameWithHash)
    const webpackAssetPath_ = (() => {
      if (webpackAssetPath.startsWith("http") || webpackAssetPath.startsWith("/")) {
        return webpackAssetPath
      } else {
        return "/" + webpackAssetPath
      }
    })()

    return Promise.resolve({
      format: "module",
      source: `export default "${webpackAssetPath_}";`,
      shortCircuit: true,
    })
  } else {
    // Defer to Node.js for all other URLs.
    return nextLoad(url)
  }
}
