module Css = Content_Css

@module("./images/cat.jpeg") external catImage: string = "default"

@module("./content.css") external css: string = "default"
css->ignore

@module("lite-flag-icon/css/flag-icon.min.css") external flagsCss: string = "default"
flagsCss->ignore

@react.component
let make = () => {
  let (isFlagVisible, setIsFlagVisible) = React.useState(() => false)

  <div className=Css.content>
    <div> {"Imported image file:"->React.string} </div>
    <div> <img src=catImage /> </div>
    <div>
      {"Flag from imported external CSS lib: "->React.string}
      <div className={"flag-icon flag-icon-es"} />
    </div>
    <div> {"Button styled via imported CSS:"->React.string} </div>
    <button
      className="customButton" onClick={_ => setIsFlagVisible(isFlagVisible => !isFlagVisible)}>
      {isFlagVisible ? "Hide USA flag"->React.string : "Show USA flag"->React.string}
    </button>
    {isFlagVisible ? <div className={"flag-icon flag-icon-us"} /> : React.null}
    <div> {"ENV_VAR: "->React.string} {Env.envVar->React.string} </div>
    <div> {"GLOBAL_VAR: "->React.string} {Env.globalVar->React.string} </div>
    <PerPageGlobals />
  </div>
}
