// We get a file's hash and make a JS module that exports a filename with hash suffix.

let getFinalHashedAssetPath =
    (url: string, processFileData: option(Buffer.t => Buffer.t)) => {
  let filePath = url->Js.String2.replace("file://", "");

  let fileData = Fs.readFileSync(filePath);

  let processedFileData =
    switch (processFileData) {
    | None => fileData
    | Some(func) => func(fileData)
    };

  let fileHash = Webpack.Hash.bufferToHash(processedFileData);

  let fileName = Path.basename(url);

  let fileExt = Path.extname(fileName);

  let filenameWithoutExt = fileName->Js.String2.replace(fileExt, "");

  let filenameWithHash = {j|$(filenameWithoutExt).$(fileHash)$(fileExt)|j};

  let webpackAssetPath =
    Path.join3(
      CliArgs.assetPrefix,
      Webpack.webpackAssetsDir,
      filenameWithHash,
    );

  let webpackAssetPath =
    if (webpackAssetPath->Js.String2.startsWith("http")
        || webpackAssetPath->Js.String2.startsWith("/")) {
      webpackAssetPath;
    } else {
      "/" ++ webpackAssetPath;
    };

  webpackAssetPath;
};

let processAsset =
    (url: string, processFileData: option(Buffer.t => Buffer.t)) => {
  let webpackAssetPath = getFinalHashedAssetPath(url, processFileData);

  Js.Promise.resolve({
    "format": "module",
    "source": {j|export default "$(webpackAssetPath)"|j},
    // shortCircuit is needed since node v16.17.0
    "shortCircuit": true,
  });
};
