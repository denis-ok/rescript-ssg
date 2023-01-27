// https://nodejs.org/api/child_process.html#child_processspawnsynccommand-args-options

type spawnSyncOutput = {status: Js.Nullable.t(int)};

[@module "child_process"]
external spawnSync': (. string, array(string), Js.t('a)) => spawnSyncOutput =
  "spawnSync";

let spawnSync = (command, args, options) => {
  let result = spawnSync'(. command, args, options);
  let exitCode =
    result.status->Js.Nullable.toOption->Belt.Option.getWithDefault(0);
  exitCode;
};
