type t;

[@val] external fromString: (string, ~encoding: string) => t = "Buffer.from";

[@send] external toString: (t, string) => string = "toString";

let fromUtf8String = s => fromString(s, ~encoding="utf8");
