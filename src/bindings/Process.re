type process;

[@val] external process: process = "process";

[@send] external exit': (process, int) => 'a = "exit";

let exit = int => process->exit'(int);
