type padding = {padding: bool};

[@mel.module "base32-encode"]
external base32Encode': (Buffer.t, string, padding) => string = "default";

let base32Encode = buffer =>
  base32Encode'(buffer, "RFC4648", {padding: false});
