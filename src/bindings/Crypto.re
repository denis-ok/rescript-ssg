module Hash = {
  type crypto;

  type hash;

  [@module "crypto"] external crypto: crypto = "default";

  [@send "createHash"]
  external createHash': (crypto, string) => hash = "createHash";

  [@send "update"]
  external updateBufferWithBuffer: (hash, Buffer.t) => hash = "update";

  [@send "update"]
  external updateBufferWithString: (hash, string) => hash = "update";

  [@send "digest"] external digest: (hash, string) => string = "digest";

  let digestLength = 20;

  let createMd4 = () => crypto->createHash'("md4");

  let bufferToHash = (data: Buffer.t) =>
    createMd4()
    ->updateBufferWithBuffer(data)
    ->digest("hex")
    ->Js.String2.slice(~from=0, ~to_=digestLength);

  let stringToHash = (data: string) =>
    createMd4()
    ->updateBufferWithString(data)
    ->digest("hex")
    ->Js.String2.slice(~from=0, ~to_=digestLength);
};