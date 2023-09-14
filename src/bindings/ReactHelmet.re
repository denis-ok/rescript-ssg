[@react.component] [@module "react-helmet"]
external make: (~children: React.element) => React.element = "Helmet";

type helmetProperty = {
  //
  toString: unit => string,
};

type helmetInstance = {
  base: helmetProperty,
  bodyAttributes: helmetProperty,
  htmlAttributes: helmetProperty,
  link: helmetProperty,
  meta: helmetProperty,
  noscript: helmetProperty,
  script: helmetProperty,
  style: helmetProperty,
  title: helmetProperty,
};

// https://github.com/nfl/react-helmet#server-usage

[@bs.module "react-helmet"] [@bs.scope "Helmet"]
external renderStatic: unit => helmetInstance = "renderStatic";
