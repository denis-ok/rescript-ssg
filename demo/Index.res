switch ReactDOM.querySelector("#app") {
| Some(root) => ReactDOM.hydrate(<Example />, root)
| None => ()
}
