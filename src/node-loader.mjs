// We need to fix the following error that appeared after bs-css added:
// /Users/denis/projects/builder/node_modules/bs-css-emotion/src/Css.bs.js:3
// import * as Curry from "rescript/lib/es6/curry.js";
// ^^^^^^

// This loader force NodeJS to load bs-artifacts as es6 module
// https://nodejs.org/api/esm.html#loaders


const isBsArtifact = url => {
  return url.match(/file:.*\.bs\.js/i)
}

export async function load(url, context, defaultLoad) {
  if (isBsArtifact(url)) {
    const format = "module";
    const { source } = await defaultLoad(url, { format });

    return {
      source,
      format,
    };
  }

  // Defer to Node.js for all other URLs.
  return defaultLoad(url, context, defaultLoad);
}
