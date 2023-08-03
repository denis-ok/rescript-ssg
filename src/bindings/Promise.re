include Js.Promise;

[@send]
external map: (Js.Promise.t('a), 'a => 'b) => Js.Promise.t('b) = "then";

[@send]
external flatMap:
  (Js.Promise.t('a), 'a => Js.Promise.t('b)) => Js.Promise.t('b) =
  "then";

[@send]
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
    ->catch(error => Belt.Result.Error((context, error))->Js.Promise.resolve);

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
