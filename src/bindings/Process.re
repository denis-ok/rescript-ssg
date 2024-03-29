type process;

external process: process = "process";

[@mel.send] external exit': (process, int) => 'a = "exit";

[@mel.get] external argv: process => array(string) = "argv";

let exit = int => process->exit'(int);

let getArgs = () => process->argv;

external env: Js.Dict.t(string) = "process.env";

[@mel.send] external on: (process, string, unit => unit) => unit = "on";

let onTerminate = callback =>
  [|"SIGINT", "SIGTERM"|]
  ->Js.Array.forEach(~f=signal => process->on(signal, callback), _);
