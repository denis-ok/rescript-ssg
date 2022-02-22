switch ReactDOM.querySelector("#app") {
| Some(root) => ReactDOM.hydrate(<App />, root)
| None => ()
}
