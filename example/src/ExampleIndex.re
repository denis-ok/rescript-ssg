module Css = ExampleIndex_Css;

let moduleName = __MODULE__;

let modulePath = Utils.getFilepath();

[@react.component]
let make = () => {
  <div>
    <ExampleHelmet
      title="ExampleIndex"
      description="ExampleIndex description"
    />
    <ExampleSharedModule.Header />
    <div className=Css.content> <h1> "Index page"->React.string </h1> </div>
    <ExampleSharedModule.Footer />
  </div>;
};
