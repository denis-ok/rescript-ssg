type process;

[@bs.val] external process: process = "process";

[@bs.send] external exit': (process, int) => 'a = "exit";

[@bs.get] external argv: process => array(string) = "argv";

let exit = int => process->exit'(int);

let getArgs = () => process->argv;

[@bs.val] external env: Js.Dict.t(string) = "process.env";
