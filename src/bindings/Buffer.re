type t;

[@val] external fromString: (string, string) => t = "Buffer.from";

[@send] external toString: (t, string) => string = "toString";
