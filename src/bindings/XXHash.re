type hash;

type encbuf = [ | `buffer | `hex | `base64 | `binary];

[@module "xxhash"] [@new]
external createXXHash64': (~seed: int) => hash = "XXHash64";

[@module "xxhash"]
external bufferTohash': (. Buffer.t, ~seed: int, ~encbuf: encbuf) => 'a =
  "hash64";

let seed = 123;

let createXXHash64 = () => createXXHash64'(~seed);

let bufferToHash = (buffer: Buffer.t) =>
  bufferTohash'(. buffer, ~seed, ~encbuf=`hex);

let stringToHash = (s: string) =>
  bufferTohash'(. Buffer.fromUtf8String(s), ~seed, ~encbuf=`hex);
