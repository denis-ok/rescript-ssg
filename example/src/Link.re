module Css = Link_Css;

[@react.component]
let make = (~href, ~children) => <a className=Css.link href> children </a>;
