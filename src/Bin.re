let dirname = Utils.getDirname();

// This module is expected to be imported when we use bin.mjs and
// all other files imported by binary are emitted with "mel.mjs" output ("install" target).
// So we have hardcoded .mel.js extension here and it should work fine.
let nodeLoaderPath = Path.join2(dirname, "./NodeLoader.mel.mjs");

let nodeOptions = [|
  {j|--experimental-loader=$(nodeLoaderPath)|j},
  "--no-warnings",
|];

let run = () => {
  switch (
    ChildProcess.spawnSync(
      "node",
      Js.Array.concat(
        ~other=Js.Array.slice(~start=2, Process.getArgs()),
        nodeOptions,
      ),
      {"shell": true, "encoding": "utf8", "stdio": "inherit"},
    )
  ) {
  | Ok () => ()
  | Error(JsError(error)) =>
    Js.Console.error2("[rescript-ssg Bin] Error:\n", error);
    Process.exit(1);
  | Error(ExitCodeIsNotZero(exitCode)) =>
    Js.Console.error2(
      "[rescript-ssg Bin] Failure! Exit code is not zero:",
      exitCode,
    );
    Process.exit(1);
  | exception (Js.Exn.Error(error)) =>
    Js.Console.error2(
      "[rescript-ssg Bin] Exception:\n",
      error->Js.Exn.message,
    );
    Process.exit(1);
  };
};
