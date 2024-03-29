[@react.component]
let make = (~children) =>
  <div> "Hello from page wrapper"->React.string children </div>;

let modulePath = Ssg.Utils.getFilepath();
