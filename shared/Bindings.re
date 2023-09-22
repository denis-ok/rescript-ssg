module Promise = {
  include Js.Promise;

  [@mel.send]
  external map: (Js.Promise.t('a), 'a => 'b) => Js.Promise.t('b) = "then";

  [@mel.send]
  external flatMap:
    (Js.Promise.t('a), 'a => Js.Promise.t('b)) => Js.Promise.t('b) =
    "then";

  [@mel.send]
  external catch:
    (Js.Promise.t('a), Js.Promise.error => Js.Promise.t('b)) =>
    Js.Promise.t('b) =
    "catch";

  let seqRun = (functions: array(unit => Js.Promise.t('a))) => {
    Js.Array2.reduce(
      functions,
      (acc, func) => {
        switch (acc) {
        | [] => [func()]
        | [promise, ...rest] => [
            promise->flatMap(_ => func()),
            promise,
            ...rest,
          ]
        }
      },
      [],
    )
    ->Belt.List.toArray
    ->Js.Promise.all;
  };

  module Result = {
    let catch =
        (promise, ~context: string)
        : Js.Promise.t(Belt.Result.t('ok, (string, Js.Promise.error))) =>
      promise
      ->map(value => Belt.Result.Ok(value))
      ->catch(error =>
          Belt.Result.Error((context, error))->Js.Promise.resolve
        );

    let all = (promises: Js.Promise.t(array(Belt.Result.t('ok, 'error)))) =>
      promises->map(promises => {
        let (oks, errors) =
          promises->Js.Array2.reduce(
            ((oks, errors), result) =>
              switch (result) {
              | Ok(ok) => (Js.Array2.concat([|ok|], oks), errors)
              | Error(error) => (oks, Js.Array2.concat([|error|], errors))
              },
            ([||], [||]),
          );

        switch (errors) {
        | [||] => Ok(oks)
        | _ => Error(errors)
        };
      });

    let map =
        (promise: Js.Promise.t(Belt.Result.t('a, 'error)), func: 'a => 'b) =>
      promise->map(result => result->Belt.Result.map(func));

    let flatMap =
        (
          promise: Js.Promise.t(Belt.Result.t('a, 'error)),
          func: 'a => Js.Promise.t('b),
        ) =>
      promise->flatMap(result =>
        switch (result) {
        | Ok(ok) => func(ok)
        | Error(error) => Js.Promise.resolve(Error(error))
        }
      );
  };
};

module ChildProcess = {
  // https://nodejs.org/api/child_process.html#child_processspawnsynccommand-args-options

  type jsError;

  type spawnSyncOutput = {
    status: Js.Nullable.t(int),
    error: Js.Nullable.t(jsError),
  };

  [@mel.module "node:child_process"]
  external spawnSync': (. string, array(string), Js.t('a)) => spawnSyncOutput =
    "spawnSync";

  type error =
    | JsError(jsError)
    | ExitCodeIsNotZero(int);

  let spawnSync = (command, args, options) => {
    let result = spawnSync'(. command, args, options);

    let jsError = result.error->Js.Nullable.toOption;

    switch (jsError) {
    | Some(e) => Belt.Result.Error(JsError(e))
    | None =>
      let exitCode =
        result.status->Js.Nullable.toOption->Belt.Option.getWithDefault(0);
      if (exitCode != 0) {
        Belt.Result.Error(ExitCodeIsNotZero(exitCode));
      } else {
        Ok();
      };
    };
  };
};

module HashWasm = {
  // https://github.com/Daninet/hash-wasm#api

  [@mel.module "hash-wasm"]
  external md5: Buffer.t => Promise.t(string) = "md5";

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
    init: (. unit) => hasher,
    update: (. Buffer.t) => hasher,
    digest: (. string) => Buffer.t,
    save: (. unit) => Buffer.t,
    load: (. Buffer.t) => hasher,
  };

  [@mel.module "hash-wasm"]
  external createXXHash64: unit => Promise.t(hasher) = "createXXHash64";

  let createXXHash64AndReturnBinaryDigest = (buffer: Buffer.t) => {
    createXXHash64()
    ->Promise.map(hasher =>
        hasher.init(.).update(. buffer).digest(. "binary")
      );
  };
};
module Path = {
  [@mel.module "path"] external join2: (string, string) => string = "join";
  [@mel.module "path"]
  external join3: (string, string, string) => string = "join";
  [@mel.module "path"] external basename: string => string = "basename";
  [@mel.module "path"] external extname: string => string = "extname";
  [@mel.module "path"] external dirname: string => string = "dirname";
  [@mel.module "path"]
  external relative: (~from: string, ~to_: string) => string = "relative";
};

module Process = {
  type process;

  external process: process = "process";

  [@mel.send] external exit': (process, int) => 'a = "exit";

  [@mel.get] external argv: process => array(string) = "argv";

  let exit = int => process->exit'(int);

  let getArgs = () => process->argv;

  external env: Js.Dict.t(string) = "process.env";
};

module Base32Encode = {
  type padding = {padding: bool};

  [@mel.module "base32-encode"]
  external base32Encode': (Buffer.t, string, padding) => string = "default";

  let base32Encode = buffer =>
    base32Encode'(buffer, "RFC4648", {padding: false});
};

module Fs = {
  [@mel.module "fs"]
  external readFileSync': (~path: string, ~encoding: string) => string =
    "readFileSync";

  [@mel.module "fs"]
  external readFileSyncAsBuffer: string => Buffer.t = "readFileSync";

  [@mel.module "fs"]
  external writeFileSync: (~path: string, ~data: string) => unit =
    "writeFileSync";

  [@mel.module "fs"] external existsSync: string => bool = "existsSync";

  type mkDirOptions = {recursive: bool};

  [@mel.module "fs"]
  external mkDirSync: (string, mkDirOptions) => unit = "mkdirSync";

  type rmSyncOptions = {
    force: bool,
    recursive: bool,
  };

  [@mel.module "fs"]
  external rmSync: (string, rmSyncOptions) => unit = "rmSync";

  let readFileSyncAsUtf8 = path => readFileSync'(~path, ~encoding="utf8");

  module Promises = {
    [@mel.module "node:fs/promises"]
    external readFileAsBuffer: string => Promise.t(Buffer.t) = "readFile";

    [@mel.module "node:fs/promises"]
    external mkDir: (string, mkDirOptions) => Promise.t(unit) = "mkdir";

    [@mel.module "node:fs/promises"]
    external writeFile: (~path: string, ~data: string) => Promise.t(unit) =
      "writeFile";
  };
};

module Util = {
  // https://nodejs.org/api/util.html#utilinspectobject-options

  type options = {
    depth: int,
    colors: bool,
  };

  [@mel.module "node:util"]
  external inspect: ('a, options) => string = "inspect";

  let inspect = value => inspect(value, {depth: 2, colors: true});
};

module Crypto = {
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
};
