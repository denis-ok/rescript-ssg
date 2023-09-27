let gracefulShutdownTimeout = 3000;

type shutdownRunningTask = unit => Js.Promise.t(unit);

let runningTasks: ref(array(shutdownRunningTask)) = ref([||]);

let addTask = (task: shutdownRunningTask) => {
  runningTasks := Js.Array2.concat([|task|], runningTasks^);
};

let shutdownRunningTasks = () =>
  (runningTasks^)->Js.Array2.map(terminate => terminate())->Promise.all;

Process.onTerminate(() => {
  Js.log("[rescript-ssg] Performing graceful shutdown...");

  shutdownRunningTasks()
  ->Promise.map(_ => {
      Js.log(
        "[rescript-ssg] Bye-bye! Graceful shutdown performed successfully",
      );
      Process.exit(0);
    })
  ->Promise.catch(error => {
      Js.Console.error2("[rescript-ssg] Graceful shutdown error:", error);
      Process.exit(1);
    })
  ->ignore;
});
