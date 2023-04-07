module Css = Index_Css

let modulePath = Utils.getFilepath()

@module("./images/cat.jpeg") external catImage: string = "default"

@module("./index.css") external css: string = "default"

@module("lite-flag-icon/css/flag-icon.min.css") external flagsCss: string = "default"

css->ignore

flagsCss->ignore

@react.component
let make = () => {
  let (isFlagVisible, setIsFlagVisible) = React.useState(() => false)

  <div>
    <MetaTags title="Index" description="Index description" />
    <SharedModule.Header />
    <div className=Css.content> <h1> {"Index page"->React.string} </h1> </div>
    <p> {"Image:"->React.string} </p>
    <div> <img src=catImage /> </div>
    <button
      className="customButton" onClick={_ => setIsFlagVisible(isFlagVisible => !isFlagVisible)}>
      {"Show USA flag"->React.string}
    </button>
    {isFlagVisible ? <div className={"flag-icon flag-icon-us"} /> : React.null}
    <SharedModule.Footer />
    <div> {"ENV_VAR: "->React.string} {Env.envVar->React.string} </div>
    <div> {"GLOBAL_VAR: "->React.string} {Env.globalVar->React.string} </div>
    <PerPageGlobal />
  </div>
}
