ExampleGlobalCss.injectGlobal();

module Header = {
  [@react.component]
  let make = () => {
    <ul>
      <li> <a href="/"> "Index"->React.string </a> </li>
      <li> <a href="/page1"> "Page 1"->React.string </a> </li>
    </ul>;
  };
};

module Footer = {
  [@react.component]
  let make = () => {
    <p> "202233"->React.string </p>;
  };
};
