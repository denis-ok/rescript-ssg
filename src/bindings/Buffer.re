type t;

[@bs.val]
external fromString: (string, ~encoding: string) => t = "Buffer.from";

[@bs.send] external toString: (t, string) => string = "toString";
