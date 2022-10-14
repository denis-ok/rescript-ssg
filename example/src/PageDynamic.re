module Css = Page1_Css;

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
    <MetaTags title="PageDynamic" description="PageDynamic description" />
    <SharedModule.Header />
    <div className=Css.content>
      <h1> "PageDynamic"->React.string </h1>
      <h2> path->React.string </h2>
    </div>
    <SharedModule.Footer />
  </div>;
};
