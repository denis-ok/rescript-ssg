[@react.component]
let make = (~moduleName, ~children) => {
  let modulesWithHydration = React.useContext(WithHydrationContext.context);

  let () = modulesWithHydration->Js.Array2.push(moduleName)->ignore;

  <> <script id={"hydrated__" ++ moduleName} /> children </>;
};
