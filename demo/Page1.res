switch ReactDOM.querySelector("#app") {
| Some(root) => ReactDOM.hydrate(<> <Component1 /> <SharedComponent /> </>, root)
| None => ()
}
