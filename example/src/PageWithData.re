let modulePath = Ssg.Utils.getFilepath();

[@react.component]
let make = (~data: option(PageContext.t)) =>
  <>
    <MetaTags title="PageWithData" description="PageWithData description" />
    <Header h1Text="PageWithData" />
    <div>
      {switch (data) {
       | None => React.null
       | Some({string, int, float, bool, variant, polyVariant, option}) =>
         <ul>
           <li> "string: "->React.string string->React.string </li>
           <li>
             "int: "->React.string
             {int->Belt.Int.toString->React.string}
           </li>
           <li>
             "float: "->React.string
             {float->Belt.Float.toString->React.string}
           </li>
           <li>
             "bool: "->React.string
             {bool->string_of_bool->React.string}
           </li>
           <li>
             "variant: "->React.string
             {switch ((variant: PageContext.variant)) {
              | One => "One"->React.string
              | Two(_) => "Two"->React.string
              }}
           </li>
           <li>
             "polyVariant: "->React.string
             {switch ((polyVariant: PageContext.polyVariant)) {
              | `hello => "hello"->React.string
              | `world => "world"->React.string
              }}
           </li>
           <li>
             "option(string): "->React.string
             {switch (option) {
              | None => "None"->React.string
              | Some(s) => ("Some " ++ s)->React.string
              }}
           </li>
         </ul>
       }}
    </div>
    <Content />
    <Footer />
  </>;
