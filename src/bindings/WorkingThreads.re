type parentPort;

type worker;

[@module "worker_threads"] external workerData: 'a = "workerData";

[@module "worker_threads"] external parentPort: parentPort = "parentPort";

module Worker = {
  type workerDataArg('a) = {workerData: 'a};

  [@new] [@module "worker_threads"]
  external make: (string, workerDataArg('a)) => worker = "Worker";

  [@send] external on: (worker, string, 'a) => unit = "on";
};

[@send] external postMessage: (parentPort, 'a) => unit = "postMessage";

let runWorker = (~workerModulePath, ~workerData: 'a) => {
  Js.Promise.make((~resolve, ~reject) => {
    let worker = Worker.make(workerModulePath, {workerData: workerData});

    worker->Worker.on("message", message => resolve(. message));
    worker->Worker.on("error", error => reject(. error));
    worker->Worker.on("exit", code => Js.log2("[Worker] Exit code:", code));
  });
};
