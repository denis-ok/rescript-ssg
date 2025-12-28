[@react.component]
let make = (~children) => <div> "Hello from page wrapper"->React.string children </div>;

let modulePath = RescriptSsg.Utils.getFilepath();
