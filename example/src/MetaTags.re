[@react.component]
let make = (~title, ~description) =>
  <Ssg.ReactHelmet>
    <html lang="en" />
    <title> title->React.string </title>
    <meta name="description" content=description />
  </Ssg.ReactHelmet>;
