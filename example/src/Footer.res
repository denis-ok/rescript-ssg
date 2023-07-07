@react.component
let make = () =>
  <p>
    {"This page has been built with "->React.string}
    <Link href="https://github.com/denis-ok/rescript-ssg"> {"rescript-ssg"->React.string} </Link>
    {" library."->React.string}
  </p>
