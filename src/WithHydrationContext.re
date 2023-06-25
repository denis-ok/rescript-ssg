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
