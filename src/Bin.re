let dirname = Utils.getDirname();

let nodeLoaderPath = Path.join2(dirname, "./NodeLoader.bs.js");

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
