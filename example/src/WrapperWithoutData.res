let modulePath = Utils.getFilepath()

@react.component
let make = (~children) => <div> <h2> {"Hello from simple page wrapper"->React.string} </h2> children </div>
