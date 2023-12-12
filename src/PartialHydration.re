let makeScriptId = (~moduleName) => {
  // TODO hashify it
  let modulePrefix = moduleName->Js.String.replaceByRe(~regexp=[%re {|/\./g|}], ~replacement="_", _);
  "withHydration__" ++ modulePrefix;
};

let renderReactAppTemplate = (~modulesWithHydration__Mutable: array(string)) => {
  modulesWithHydration__Mutable
  ->Js.Array.map(~f=moduleName => {
      let scriptId = makeScriptId(~moduleName);
      {j|
switch (ReactDOM.querySelector("#$(scriptId)")) {
| Some(root) => ReactDOM.hydrate(<$(moduleName) />, root)
| None => ()
};
|j};
    }, _)
  ->Js.Array.join(~sep="\n", _);
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

    let () = modulesWithHydration->Js.Array.push(~value=moduleName)->ignore;

    <div id={makeScriptId(~moduleName)}> children </div>;
  };
};
