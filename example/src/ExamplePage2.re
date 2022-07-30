let moduleName = __MODULE__;

let modulePath = Utils.getFilepath();

[@react.component]
let make = (~data: bool) => {
  <>
    <ExampleHelmet
      title="ExamplePage2"
      description="ExamplePage2 description"
    />
    <ExampleSharedModule.Header />
    {switch (data) {
     | true => "True"->React.string
     | _false => "False"->React.string
     }}
    <ExampleSharedModule.Footer />
  </>;
};
