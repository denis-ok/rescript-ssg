[@module "fs"]
external readFileSync: (string, string) => string = "readFileSync";

[@module "fs"]
external writeFileSync: (string, string) => unit = "writeFileSync";

type makeDirSyncOptions = {recursive: bool};

[@module "fs"]
external mkDirSync: (string, makeDirSyncOptions) => unit = "mkdirSync";

type rmSyncOptions = {
  force: bool,
  recursive: bool,
};

[@module "fs"] external rmSync: (string, rmSyncOptions) => unit = "rmSync";
