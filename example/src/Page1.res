module Css = Page1_Css

let modulePath = Utils.getFilepath()

@react.component
let make = (~data: option<PageContext.t>) =>
  <div>
    {switch data {
    | None => React.null
    | Some({string, int, float, variant, polyVariant, option, bool}) =>
      <ul>
        <li> {bool->string_of_bool->React.string} </li>
        <li> {bool->string_of_bool->React.string} </li>
        <li> {string->React.string} </li>
        <li> {int->Belt.Int.toString->React.string} </li>
        <li> {float->Belt.Float.toString->React.string} </li>
        <li>
          {switch (variant: PageContext.variant) {
          | One => "One"->React.string
          | Two(_) => "Two"->React.string
          }}
        </li>
        <li>
          {switch (polyVariant: PageContext.polyVariant) {
          | #hello => "hello"->React.string
          | #world => "world"->React.string
          }}
        </li>
        <li>
          {switch option {
          | None => "None"->React.string
          | Some(s) => ("Some " ++ s)->React.string
          }}
        </li>
      </ul>
    }}
    <MetaTags title="Page1" description="Page1 description" />
    <SharedModule.Header />
    <div className=Css.content> <h1> {"Page 1"->React.string} </h1> </div>
    <SharedModule.Footer />
  </div>
