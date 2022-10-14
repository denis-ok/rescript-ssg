[@module "child_process"]
external execSync: (. string, Js.t('a)) => string = "execSync";

module Error = {
  [@get] external stdout: Js.Exn.t => string = "stdout";
};
