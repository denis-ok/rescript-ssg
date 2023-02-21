type jsesc;

type options = {
  quotes: string,
  json: bool,
  wrap: bool,
  es6: bool,
};

[@module "jsesc"] external jsesc: ('a, options) => string = "default";

let jsesc = a =>
  jsesc(a, {quotes: "double", json: false, wrap: true, es6: false});
