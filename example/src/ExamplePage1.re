let moduleName = __MODULE__;

let modulePath = Utils.getFilepath();

[@react.component]
let make = () => {
  <div> "Page 1"->React.string <ExampleSharedModule /> </div>;
};
