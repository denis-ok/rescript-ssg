switch (ReactDOM.querySelector("#rsdfoot")) {
| Some(root) => ReactDOM.Client.hydrateRoot(root, <div />)->ignore
| None => ()
};
