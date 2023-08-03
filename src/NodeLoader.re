[@send] external replaceAll: (string, string, string) => string = "replaceAll";

// 'v16.15.0' => 16150
let nodeVersionToInt = (s: string) => {
  let refinedNodeVersion = s->replaceAll("v", "")->replaceAll(".", "");
  Belt.Int.fromString(refinedNodeVersion)->Belt.Option.getWithDefault(0);
};

let bsArtifactRegex = [%re {|/file:.*\.bs\.js$/i|}];

let isBsArtifact = fileUrl => {
  Js.String2.match(fileUrl, bsArtifactRegex) != None;
};

let isAsset = fileUrl => {
  Js.String2.match(fileUrl, Bundler.assetRegex) != None;
};

// We get a file's hash and make a JS module that exports a filename with hash suffix.
let getFinalHashedAssetPath =
    (url: string, processFileData: option(Buffer.t => Buffer.t)) => {
  let filePath = url->Js.String2.replace("file://", "");

  filePath
  ->Fs.Promises.readFileAsBuffer
  ->Promise.catch(error => {
      Js.Console.error2(
        "[NodeLoader.getFinalHashedAssetPath] [Fs.Promises.readFileAsBuffer] Error:",
        error,
      );
      Process.exit(1);
    })
  ->Promise.flatMap(fileData => {
      let processedFileData =
        switch (processFileData) {
        | None => fileData
        | Some(func) => func(fileData)
        };

      let fileName = Path.basename(url);

      let fileExt = Path.extname(fileName);

      let filenameWithoutExt = fileName->Js.String2.replace(fileExt, "");

      let filenameWithHash =
        switch (Bundler.bundler) {
        | Webpack =>
          let fileHash = Crypto.Hash.bufferToHash(processedFileData);
          Promise.resolve(filenameWithoutExt ++ "." ++ fileHash ++ fileExt);
        | Esbuild =>
          // cat-FU5UU3XL.jpeg
          Esbuild.getFileHash(processedFileData)
          ->Promise.map(fileHash => {
              filenameWithoutExt ++ "-" ++ fileHash ++ fileExt
            })
          ->Promise.catch(error => {
              Js.Console.error2(
                "[NodeLoader.getFinalHashedAssetPath] [Esbuild.getFileHash] Error:",
                error,
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
        error,
      );
      Process.exit(1);
    });
};

let makeAssetSource = (webpackAssetPath: string) => {j|export default "$(webpackAssetPath)"|j};

let processAsset =
    (url: string, processFileData: option(Buffer.t => Buffer.t)) => {
  getFinalHashedAssetPath(url, processFileData)
  ->Promise.map(webpackAssetPath =>
      {
        "format": "module",
        "source": makeAssetSource(webpackAssetPath),
        "shortCircuit": true,
      }
    );
};
