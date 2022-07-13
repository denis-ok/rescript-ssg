module Css = ExamplePage1_Css;

let moduleName = __MODULE__;

let modulePath = Utils.getFilepath();

[@react.component]
let make = () => {
  let url =
    RescriptReactRouter.useUrl(
      ~serverUrl={path: ["page1", "@id"], hash: "", search: ""},
      (),
    );

  let path = url.path->Belt.List.toArray->Js.Array2.joinWith("/");

  <div>
    <ExampleHelmet
      title="ExamplePageDynamic"
      description="ExamplePageDynamic description"
    />
    <ExampleSharedModule.Header />
    <div className=Css.content>
      <h1> "ExamplePageDynamic"->React.string </h1>
      <h2> path->React.string </h2>
    </div>
    <ExampleSharedModule.Footer />
  </div>;
};
