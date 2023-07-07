let modulePath = Utils.getFilepath()

@react.component
let make = (~data, ~children) =>
  <div>
    <h2> {"Hello from page wrapper with data"->React.string} </h2>
    <h3> {"Data: "->React.string} </h3>
    <h3> {data->React.string} </h3>
    children
  </div>
