[@module "fs"]
external readFileSync': (~path: string, ~encoding: string) => string =
  "readFileSync";

[@module "fs"]
external readFileSyncAsBuffer: string => Buffer.t = "readFileSync";

[@module "fs"]
external writeFileSync: (~path: string, ~data: string) => unit =
  "writeFileSync";

[@module "fs"] external existsSync: string => bool = "existsSync";

type makeDirSyncOptions = {recursive: bool};

[@module "fs"]
external mkDirSync: (string, makeDirSyncOptions) => unit = "mkdirSync";

type rmSyncOptions = {
  force: bool,
  recursive: bool,
};

[@module "fs"] external rmSync: (string, rmSyncOptions) => unit = "rmSync";

let readFileSyncAsUtf8 = path => readFileSync'(~path, ~encoding="utf8");

module Promises = {
  [@module "node:fs/promises"]
  external readFileAsBuffer': string => Promise.t(Buffer.t) = "readFile";

  let readFileAsBuffer = path => path->readFileAsBuffer'->Promise.toResult;
};
