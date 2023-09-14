type parentPort;

type worker;

[@bs.module "worker_threads"] external workerData: 'a = "workerData";

[@bs.module "worker_threads"] external parentPort: parentPort = "parentPort";

module Worker = {
  type workerDataArg('a) = {workerData: 'a};

  [@bs.new] [@bs.module "worker_threads"]
  external make: (string, workerDataArg('a)) => worker = "Worker";

  [@bs.send] external on: (worker, string, 'a) => unit = "on";
};

[@bs.send] external postMessage: (parentPort, 'a) => unit = "postMessage";

// Compiler doesn't know what will be returned by runWorker function.
// We need to carefully annotate the call of this function in place.
let runWorker = (~workerModulePath, ~workerData: 'a, ~onExit: int => unit) => {
  Promise.make((~resolve, ~reject) => {
    let worker = Worker.make(workerModulePath, {workerData: workerData});

    worker->Worker.on("message", message => resolve(. message));
    worker->Worker.on("error", error => reject(. error));
    worker->Worker.on("exit", exitCode => onExit(exitCode));
  });
};
