@react.component
let make = (~data, ~children) =>
  <div>
    <h2> {"Hello from page wrapper with data:"->React.string} </h2>
    <h2> {data->React.string} </h2>
    children
  </div>

let modulePath = Utils.getFilepath()
