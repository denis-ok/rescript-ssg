[@send]
external map: (Js.Promise.t('a), 'a => 'b) => Js.Promise.t('b) = "then";

let bind = (callback, promise) => Js.Promise.then_(promise, callback);

let catch = (callback, promise) => Js.Promise.catch(promise, callback);

let seqRun = (functions: array(unit => Js.Promise.t('a))) => {
  Js.Array2.reduce(
    functions,
    (acc, func) => {
      switch (acc) {
      | [] => [func()]
      | [promise, ...rest] => [promise->bind(_ => func()), promise, ...rest]
      }
    },
    [],
  )
  ->Belt.List.toArray
  ->Js.Promise.all;
};

let toResult = promise =>
  promise
  ->map(value => Belt.Result.Ok(value))
  ->catch(error => Belt.Result.Error(error)->Js.Promise.resolve);
