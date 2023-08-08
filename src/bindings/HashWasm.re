// https://github.com/Daninet/hash-wasm#api

[@module "hash-wasm"] external md5: Buffer.t => Js.Promise.t(string) = "md5";

// interface IHasher {
//   init: () => IHasher;
//   update: (data: IDataType) => IHasher;
//   digest: (outputType: 'hex' | 'binary') => string | Uint8Array; // by default returns hex string
//   save: () => Uint8Array; // returns the internal state for later resumption
//   load: (state: Uint8Array) => IHasher; // loads a previously saved internal state
//   blockSize: number; // in bytes
//   digestSize: number; // in bytes
// }

type hasher = {
  init: unit => hasher,
  update: Buffer.t => hasher,
  digest: string => Buffer.t,
  save: unit => Buffer.t,
  load: Buffer.t => hasher,
};

[@module "hash-wasm"]
external createXXHash64:
  (~seedLow: option(int), ~seedHigh: option(int)) => Js.Promise.t(hasher) =
  "createXXHash64";

let createXXHash64AndReturnBinaryDigest = (. buffer: Buffer.t) => {
  createXXHash64(~seedLow=None, ~seedHigh=None)
  ->Promise.map(hasher => hasher.init())
  ->Promise.map(hasher => hasher.update(buffer))
  ->Promise.map(hasher => hasher.digest("binary"));
};
