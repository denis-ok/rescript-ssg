module Css = ExampleIndex_Css;

let moduleName = __MODULE__;

let modulePath = Utils.getFilepath();

[@react.component]
let make = () => {
  <div className=Css.container>
    "Index page"->React.string
    <ExampleSharedModule />
  </div>;
};
