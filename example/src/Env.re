external envVar: option(string) = "process.env.ENV_VAR";

let envVar = envVar->(Belt.Option.getWithDefault("ENV_VAR IS MISSING"));

external globalVar: string = "GLOBAL_VAR";
