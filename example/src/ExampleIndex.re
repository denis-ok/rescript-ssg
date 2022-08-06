module Css = ExampleIndex_Css;

let modulePath = Utils.getFilepath();

let moduleName = Utils.getModuleNameFromModulePath(modulePath);

[@module "./images/cat.jpeg"] external catImage: string = "default";

[@react.component]
let make = () => {
  <div>
    <ExampleHelmet
      title="ExampleIndex"
      description="ExampleIndex description"
    />
    <ExampleSharedModule.Header />
    <div className=Css.content> <h1> "Index page"->React.string </h1> </div>
    <img src=catImage />
    <ExampleSharedModule.Footer />
  </div>;
};
