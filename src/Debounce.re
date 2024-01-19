let debounce = (~delayMs: int, func: unit => unit) => {
  let timeoutId = ref(None);

  let cancel = () => {
    (timeoutId^)
    ->Belt.Option.forEach(timeoutId => Js.Global.clearTimeout(timeoutId));
    timeoutId := None;
  };

  let schedule = () => {
    cancel();
    timeoutId :=
      Some(
        Js.Global.setTimeout(
          ~f=() => {
            func();
            timeoutId := None;
          },
          delayMs,
        ),
      );
  };

  let debounced = () => schedule();

  debounced;
};
