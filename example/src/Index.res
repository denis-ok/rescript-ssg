module Css = Index_Css

let modulePath = Utils.getFilepath()

@module("./images/cat.jpeg") external catImage: string = "default"

@react.component
let make = () =>
  <div>
    <MetaTags title="Index" description="Index description" />
    <SharedModule.Header />
    <div className=Css.content> <h1> {"Index page"->React.string} </h1> </div>
    <img src=catImage />
    <SharedModule.Footer />
  </div>
