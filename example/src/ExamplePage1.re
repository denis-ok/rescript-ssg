module Css = ExamplePage1_Css;

let moduleName = __MODULE__;

let modulePath = Utils.getFilepath();

[@react.component]
let make = (~pageContext: option(ExamplePageContext.t)) => {
  <div>
    {switch (pageContext) {
     | None => React.null
     | Some({string, int, float, variant, polyVariant, option, bool}) =>
       <ul>
         <li> {bool->string_of_bool->React.string} </li>
         <li> {bool->string_of_bool->React.string} </li>
         <li> string->React.string </li>
         <li> {int->Belt.Int.toString->React.string} </li>
         <li> {float->Belt.Float.toString->React.string} </li>
         <li>
           {switch ((variant: ExamplePageContext.variant)) {
            | One => "One"->React.string
            | Two(_) => "Two"->React.string
            }}
         </li>
         <li>
           {switch ((polyVariant: ExamplePageContext.polyVariant)) {
            | `hello => "hello"->React.string
            | `world => "world"->React.string
            }}
         </li>
         <li>
           {switch (option) {
            | None => "None"->React.string
            | Some(s) => ("Some " ++ s)->React.string
            }}
         </li>
       </ul>
     }}
    <ExampleHelmet
      title="ExamplePage1"
      description="ExamplePage1 description"
    />
    <ExampleSharedModule.Header />
    <div className=Css.content> <h1> "Page 1"->React.string </h1> </div>
    <ExampleSharedModule.Footer />
  </div>;
};
