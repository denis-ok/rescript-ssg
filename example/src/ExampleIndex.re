let moduleName = __MODULE__;

let modulePath = Utils.getFilepath();

[@react.component]
let make = () => {
  <div> "Index page"->React.string <ExampleSharedModule /> </div>;
};
