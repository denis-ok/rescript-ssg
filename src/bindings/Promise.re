[@send]
external map: (Js.Promise.t('a), 'a => 'b) => Js.Promise.t('b) = "then";

let bind = (callback, promise) => Js.Promise.then_(promise, callback);

let catch = (callback, promise) => Js.Promise.catch(promise, callback);
