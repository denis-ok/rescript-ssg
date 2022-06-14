// We need to fix the following error that appeared after bs-css added:
// /Users/denis/projects/builder/node_modules/bs-css-emotion/src/Css.bs.js:3
// import * as Curry from "rescript/lib/es6/curry.js";
// ^^^^^^

// This loader force NodeJS to load bs-artifacts as es6 module
// https://nodejs.org/api/esm.html#loaders


import path from "path"
import {webpackAssetsDir} from "./Webpack.bs.js"
// import { dirname } from 'path';
// import { fileURLToPath } from 'url';
// const __dirname = dirname(fileURLToPath(import.meta.url));

import crypto from 'crypto'

import fs from 'fs'

const makeHash = data => crypto.createHash('md4').update(data).digest("hex");

const isBsArtifact = url => {
  return url.match(/file:.*\.bs\.js/i)
}

const isJpeg = url => {
  return url.match(/file:.*\.jpg|jpeg/i)
}

export async function load(url, context, defaultLoad) {
  if (isBsArtifact(url)) {
    const format = "module";
    const { source } = await defaultLoad(url, { format });

    return {
      source,
      format,
    };
  } else if (isJpeg(url)) {
    const filepath = url.replace("file://", "")
    const filedata = fs.readFileSync(filepath)
    const filehash = makeHash(filedata).slice(0,20)
    const filename = path.basename(url)
    const fileExt = path.extname(filename)
    const filenameWithoutExt = filename.replace(fileExt, "")
    const filenameWithHash = `${filenameWithoutExt}.${filehash}${fileExt}`
    const webpackAssetPath = webpackAssetsDir + "/" + filenameWithHash;
    return Promise.resolve({
      source: `export default "${webpackAssetPath}";`,
      format: "module",
    })
  }

  // Defer to Node.js for all other URLs.
  return defaultLoad(url, context, defaultLoad);
}
