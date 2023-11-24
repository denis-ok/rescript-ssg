[@mel.send]
external replaceAll: (string, string, string) => string = "replaceAll";

let bsArtifactRegex = [%re {|/file:.*\.bs\.js$/i|}];

let isBsArtifact = fileUrl => {
  switch (Js.String2.match(fileUrl, bsArtifactRegex)) {
  | Some(_) => true
  | None => false
  };
};

let isAsset = fileUrl => {
  switch (Js.String2.match(fileUrl, Bundler.assetRegex)) {
  | Some(_) => true
  | None => false
  };
};

// Getting a hash of the file contents the same way as it implemented in esbuild.
// 1. Calc xxhash64 from binary data and return digest as binary data.
// Source: https://github.com/evanw/esbuild/blob/main/internal/bundler/bundler.go#L2191-L2195
// 2. Encode binary data to base32.
// Source: https://github.com/evanw/esbuild/blob/main/internal/bundler/bundler.go#L1084-L1086
// Side note: the author of esbuild doesn't want to export hash function to use it from js.
// Maybe it makes sense to raise this question again.
// Source: https://github.com/evanw/esbuild/issues/3113#issuecomment-1542394482
let getEsbuildFileHash = (buffer: Buffer.t) => {
  HashWasm.createXXHash64AndReturnBinaryDigest(buffer)
  ->Promise.map(buffer => {
      Base32Encode.base32Encode(buffer)->Js.String2.slice(~from=0, ~to_=8)
    });
};

// We get a file's hash and make a JS module that exports a filename with hash suffix.
let getFinalHashedAssetPath = (url: string) => {
  let filePath = url->Js.String2.replace("file://", "");

  filePath
  ->Fs.Promises.readFileAsBuffer
  ->Promise.catch(error => {
      Js.Console.error2(
        "[NodeLoader.getFinalHashedAssetPath] [Fs.Promises.readFileAsBuffer] Error:",
        error->Util.inspect,
      );
      Process.exit(1);
    })
  ->Promise.flatMap(fileData => {
      let fileName = Path.basename(url);

      let fileExt = Path.extname(fileName);

      let filenameWithoutExt = fileName->Js.String2.replace(fileExt, "");

      let filenameWithHash =
        switch (Bundler.bundler) {
        | Webpack =>
          let fileHash = Crypto.Hash.bufferToHash(fileData);
          Promise.resolve(filenameWithoutExt ++ "." ++ fileHash ++ fileExt);
        | Esbuild =>
          // cat-FU5UU3XL.jpeg
          getEsbuildFileHash(fileData)
          ->Promise.map(fileHash => {
              filenameWithoutExt ++ "-" ++ fileHash ++ fileExt
            })
          ->Promise.catch(error => {
              Js.Console.error2(
                "[NodeLoader.getFinalHashedAssetPath] [Esbuild.getFileHash] Error:",
                error->Util.inspect,
              );
              Process.exit(1);
            })
        };

      filenameWithHash->Promise.map(filenameWithHash => {
        let assetPath =
          switch (EnvParams.assetPrefix->Js.String2.startsWith("https://")) {
          | false =>
            let assetsDir =
              Path.join2(EnvParams.assetPrefix, Bundler.assetsDirname);
            Path.join2(assetsDir, filenameWithHash);
          | true =>
            let assetsDir =
              EnvParams.assetPrefix ++ "/" ++ Bundler.assetsDirname;
            assetsDir ++ "/" ++ filenameWithHash;
          };

        let assetPath = Utils.maybeAddSlashPrefix(assetPath);
        assetPath;
      });
    })
  ->Promise.catch(error => {
      Js.Console.error2(
        "[NodeLoader.getFinalHashedAssetPath] Unexpected promise rejection:",
        error->Util.inspect,
      );
      Process.exit(1);
    });
};

let makeAssetSource = (webpackAssetPath: string) => {j|export default "$(webpackAssetPath)"|j};

let processAsset = (url: string) => {
  getFinalHashedAssetPath(url)
  ->Promise.map(webpackAssetPath =>
      {
        "format": "module",
        "source": makeAssetSource(webpackAssetPath),
        "shortCircuit": true,
      }
    );
};

let load =
    (
      url,
      _context,
      nextLoad:
        (. string, option({. "format": string})) =>
        Js.Promise.t({
          .
          "format": string,
          "shortCircuit": bool,
          "source": string,
        }),
    ) =>
  if (isBsArtifact(url)) {
    // We need to fix the error that appeared after bs-css added:
    // /Users/denis/projects/builder/node_modules/bs-css-emotion/src/Css.bs.js:3
    // import * as Curry from "rescript/lib/es6/curry.js";
    // ^^^^^^
    // SyntaxError: Cannot use import statement outside a module
    // We force NodeJS to load bs-artifacts as es6 modules
    let format = "module";
    nextLoad(. url, Some({"format": format}));
  } else if (isAsset(url)) {
    processAsset(url);
  } else {
    nextLoad(. url, None);
  };
