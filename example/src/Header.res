Global_Css.injectGlobal()

@react.component
let make = (~h1Text) =>
  <div>
    <h1> {h1Text->React.string} </h1>
    <h3> {"Page examples:"->React.string} </h3>
    <ul>
      <li> <a href="/"> <h3> {"root (same as page-without-data)"->React.string} </h3> </a> </li>
      {Page.all
      ->Js.Array2.map(page => {
        let slug = Page.toSlug(page)
        <li key=slug> <a href={"/" ++ slug}> <h3> {slug->React.string} </h3> </a> </li>
      })
      ->React.array}
      <li> <a href={"/" ++ {Page.toSlug(PageWithoutData)} ++ "/some_path"}> <h3> {"page-without-data/dynamic__path"->React.string} </h3> </a> </li>
    </ul>
  </div>
