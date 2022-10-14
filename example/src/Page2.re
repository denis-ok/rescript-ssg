let modulePath = Utils.getFilepath();

[@react.component]
let make = (~data: bool) => {
  <>
    <MetaTags title="Page2" description="Page2 description" />
    <SharedModule.Header />
    {switch (data) {
     | true => "True"->React.string
     | _false => "False"->React.string
     }}
    <SharedModule.Footer />
  </>;
};
