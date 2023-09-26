type t;

external fromString: (string, ~encoding: string) => t = "Buffer.from";

[@send] external toString: (t, string) => string = "toString";
