type shutdownRunningTask = unit => Js.Promise.t(unit);

let runningTasks: ref(array(shutdownRunningTask)) = ref([||]);

let addTask = (task: shutdownRunningTask) => {
  runningTasks := Js.Array2.concat([|task|], runningTasks^);
};

let shutdownRunningTasks = () =>
  (runningTasks^)->Js.Array2.map(terminate => terminate())->Promise.all;

Process.onTerminate(() => {
  shutdownRunningTasks()
  ->Promise.map(_ => {
      Js.log("[rescript-ssg] All running tasks terminated");
      Process.exit(0);
    })
  ->Promise.catch(error => {
      Js.Console.error2("[rescript-ssg] Tasks terminated with error:", error);
      Process.exit(1);
    })
  ->ignore
});
