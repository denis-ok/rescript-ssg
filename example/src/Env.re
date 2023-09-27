[@val] external envVar: option(string) = "process.env.ENV_VAR";

let envVar = envVar->(Belt.Option.getWithDefault("ENV_VAR IS MISSING"));

[@val] external globalVar: string = "GLOBAL_VAR";
