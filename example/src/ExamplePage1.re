module Css = ExamplePage1_Css;

let moduleName = __MODULE__;

let modulePath = Utils.getFilepath();

[@react.component]
let make = () => {
  <div>
    <ExampleSharedModule.Header />
    <div className=Css.content> <h1> "Page 1"->React.string </h1> </div>
    <ExampleSharedModule.Footer />
  </div>;
};
