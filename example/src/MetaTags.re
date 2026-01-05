[@react.component]
let make = (~title, ~description) =>
  <RescriptSsg.ReasonReactHelmet>
    <html lang="en" />
    <title> title->React.string </title>
    <meta name="description" content=description />
  </RescriptSsg.ReasonReactHelmet>;
