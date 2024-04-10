[@react.component]
let make = (~title, ~description) =>
  <RescriptSsg.ReactHelmet>
    <html lang="en" />
    <title> title->React.string </title>
    <meta name="description" content=description />
  </RescriptSsg.ReactHelmet>;
