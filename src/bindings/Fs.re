[@module "fs"]
external readFileSync': (string, string) => string = "readFileSync";

[@module "fs"]
external readFileSyncAsBuffer: string => Buffer.t = "readFileSync";

[@module "fs"]
external writeFileSync: (string, string) => unit = "writeFileSync";

[@module "fs"] external existsSync: string => bool = "existsSync";

type makeDirSyncOptions = {recursive: bool};

[@module "fs"]
external mkDirSync: (string, makeDirSyncOptions) => unit = "mkdirSync";

type rmSyncOptions = {
  force: bool,
  recursive: bool,
};

[@module "fs"] external rmSync: (string, rmSyncOptions) => unit = "rmSync";

let readFileSyncAsUtf8 = path => readFileSync'(path, "utf8");
