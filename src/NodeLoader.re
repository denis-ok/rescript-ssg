module Hash = {
  type crypto;

  type hash;

  [@module "crypto"] external crypto: crypto = "default";

  [@send "createHash"]
  external createHash': (crypto, string) => hash = "createHash";

  [@send "update"] external updateBuffer: (hash, Buffer.t) => hash = "update";

  [@send "digest"] external digest: (hash, string) => string = "digest";

  let digestLength = 20;

  let makeNew = () => crypto->createHash'("md4");

  let bufferToHash = (data: Buffer.t) =>
    crypto
    ->createHash'("md4")
    ->updateBuffer(data)
    ->digest("hex")
    ->Js.String2.slice(~from=0, ~to_=digestLength);
};

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

let assetRegex = [%re
  {|/\.(css|jpg|jpeg|png|gif|svg|ico|avif|webp|woff|woff2|json|mp4)$/i|}
];

let isAsset = fileUrl => {
  Js.String2.match(fileUrl, assetRegex) != None;
};

let webpackAssetsDir = "assets";

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

  let fileHash = Hash.bufferToHash(processedFileData);

  let fileName = Path.basename(url);

  let fileExt = Path.extname(fileName);

  let filenameWithoutExt = fileName->Js.String2.replace(fileExt, "");

  let filenameWithHash = {j|$(filenameWithoutExt).$(fileHash)$(fileExt)|j};

  let webpackAssetPath =
    Path.join3(CliArgs.assetPrefix, webpackAssetsDir, filenameWithHash);

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
