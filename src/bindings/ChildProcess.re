// https://nodejs.org/api/child_process.html#child_processspawnsynccommand-args-options

type jsError;

type spawnSyncOutput = {
  status: Js.Nullable.t(int),
  error: Js.Nullable.t(jsError),
};

[@mel.module "node:child_process"]
external spawnSync': (. string, array(string), Js.t('a)) => spawnSyncOutput =
  "spawnSync";

type error =
  | JsError(jsError)
  | ExitCodeIsNotZero(int);

let spawnSync = (command, args, options) => {
  let result = spawnSync'(. command, args, options);

  let jsError = result.error->Js.Nullable.toOption;

  switch (jsError) {
  | Some(e) => Belt.Result.Error(JsError(e))
  | None =>
    let exitCode =
      result.status->Js.Nullable.toOption->Belt.Option.getWithDefault(0);
    if (exitCode != 0) {
      Belt.Result.Error(ExitCodeIsNotZero(exitCode));
    } else {
      Ok();
    };
  };
};
