Global_Css.injectGlobal()

module Header = {
  @react.component
  let make = () =>
    <ul>
      <li> <a href="/"> {"Index"->React.string} </a> </li>
      <li> <a href="/page1"> {"Page 1"->React.string} </a> </li>
    </ul>
}

module Footer = {
  @react.component
  let make = () =>
    <div>
      <p>
        {"This page has been built with "->React.string}
        <Link href="https://github.com/denis-ok/rescript-ssg">
          {"Rescript SSG"->React.string}
        </Link>
        {" library."->React.string}
      </p>
      <p> {"2022"->React.string} </p>
    </div>
}
