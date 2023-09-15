[@mel.module "fs"]
external readFileSync': (~path: string, ~encoding: string) => string =
  "readFileSync";

[@mel.module "fs"]
external readFileSyncAsBuffer: string => Buffer.t = "readFileSync";

[@mel.module "fs"]
external writeFileSync: (~path: string, ~data: string) => unit =
  "writeFileSync";

[@mel.module "fs"] external existsSync: string => bool = "existsSync";

type mkDirOptions = {recursive: bool};

[@mel.module "fs"]
external mkDirSync: (string, mkDirOptions) => unit = "mkdirSync";

type rmSyncOptions = {
  force: bool,
  recursive: bool,
};

[@mel.module "fs"] external rmSync: (string, rmSyncOptions) => unit = "rmSync";

let readFileSyncAsUtf8 = path => readFileSync'(~path, ~encoding="utf8");

module Promises = {
  [@mel.module "node:fs/promises"]
  external readFileAsBuffer: string => Promise.t(Buffer.t) = "readFile";

  [@mel.module "node:fs/promises"]
  external mkDir: (string, mkDirOptions) => Promise.t(unit) = "mkdir";

  [@mel.module "node:fs/promises"]
  external writeFile: (~path: string, ~data: string) => Promise.t(unit) =
    "writeFile";
};