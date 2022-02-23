switch ReactDOM.querySelector("#app") {
| Some(root) => ReactDOM.hydrate(<> <Component2 /> <SharedComponent /> </>, root)
| None => ()
}
