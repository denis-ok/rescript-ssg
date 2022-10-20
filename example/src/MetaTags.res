@react.component
let make = (~title, ~description) =>
  <ReactHelmet>
    <html lang="en" />
    <title> {title->React.string} </title>
    <meta name="description" content=description />
  </ReactHelmet>
