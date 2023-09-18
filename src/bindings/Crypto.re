module Hash = {
  type crypto;

  type hash;

  [@mel.module "crypto"] external crypto: crypto = "default";

  [@mel.send "createHash"]
  external createHash': (crypto, string) => hash = "createHash";

  [@mel.send "update"]
  external updateBufferWithBuffer: (hash, Buffer.t) => hash = "update";

  [@mel.send "update"]
  external updateBufferWithString: (hash, string) => hash = "update";

  [@mel.send "digest"] external digest: (hash, string) => string = "digest";

  let digestLength = 20;

  let createMd5 = () => crypto->createHash'("md5");

  let bufferToHash = (data: Buffer.t) =>
    createMd5()
    ->updateBufferWithBuffer(data)
    ->digest("hex")
    ->Js.String2.slice(~from=0, ~to_=digestLength);

  let stringToHash = (data: string) =>
    createMd5()
    ->updateBufferWithString(data)
    ->digest("hex")
    ->Js.String2.slice(~from=0, ~to_=digestLength);
};
