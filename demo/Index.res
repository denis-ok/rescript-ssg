switch ReactDOM.querySelector("#app") {
| Some(root) =>
  ReactDOM.hydrate(<div> <p> {"Hello from index page"->React.string} </p> <Component1 /> <Component2 /> <SharedComponent /> </div>, root)
| None => ()
}
