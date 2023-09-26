let makeScriptId = (~moduleName) => {
  // TODO hashify it
  let modulePrefix = moduleName->Js.String2.replaceByRe([%re {|/\./g|}], "_");
  "withHydration__" ++ modulePrefix;
};

let renderReactAppTemplate = (~modulesWithHydration__Mutable: array(string)) => {
  modulesWithHydration__Mutable
  ->Js.Array2.map(moduleName => {
      let scriptId = makeScriptId(~moduleName);
      {j|
switch (ReactDOM.querySelector("#$(scriptId)")) {
| Some(root) => ReactDOM.hydrate(<$(moduleName) />, root)
| None => ()
}
|j};
    })
  ->Js.Array2.joinWith("\n");
};

module WithHydrationContext = {
  let default: array(string) = [||];

  let context = React.createContext(default);

  module Provider = {
    let provider = React.Context.provider(context);

    [@react.component]
    let make = (~modulesWithHydration__Mutable: array(string), ~children) => {
      React.createElement(
        provider,
        {"value": modulesWithHydration__Mutable, "children": children},
      );
    };
  };
};

module WithHydration = {
  [@react.component]
  let make = (~moduleName, ~children) => {
    let modulesWithHydration = React.useContext(WithHydrationContext.context);

    let () = modulesWithHydration->Js.Array2.push(moduleName)->ignore;

    <div id={makeScriptId(~moduleName)}> children </div>;
  };
};
