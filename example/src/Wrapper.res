@react.component
let make = (~children) => <div> <h2> {"Hello from page wrapper"->React.string} </h2> children </div>

let modulePath = Utils.getFilepath()
