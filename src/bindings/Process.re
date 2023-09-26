type process;

external process: process = "process";

[@send] external exit': (process, int) => 'a = "exit";

[@get] external argv: process => array(string) = "argv";

let exit = int => process->exit'(int);

let getArgs = () => process->argv;

external env: Js.Dict.t(string) = "process.env";
