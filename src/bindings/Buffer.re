type t;

[@mel.val] external fromString: (string, ~encoding: string) => t = "Buffer.from";

[@mel.send] external toString: (t, string) => string = "toString";
