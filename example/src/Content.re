module Css = Content_Css;

[@mel.module "./images/cat.jpeg"] external catImage: string = "default";

[@mel.module "./content.css"] external css: string = "default";
css->ignore;

[@mel.module "lite-flag-icon/css/flag-icon.min.css"]
external flagsCss: string = "default";
flagsCss->ignore;

[@react.component]
let make = () => {
  let (isFlagVisible, setIsFlagVisible) = React.useState(() => false);

  <div className=Css.content>
    <div> "Imported image file:"->React.string </div>
    <div> <img src=catImage /> </div>
    <div>
      "Flag from imported external CSS lib: "->React.string
      <div className="flag-icon flag-icon-es" />
    </div>
    <div> "Button styled via imported CSS:"->React.string </div>
    <button
      className="customButton"
      onClick={_ => setIsFlagVisible(isFlagVisible => !isFlagVisible)}>
      {if (isFlagVisible) {
         "Hide USA flag"->React.string;
       } else {
         "Show USA flag"->React.string;
       }}
    </button>
    {if (isFlagVisible) {
       <div className="flag-icon flag-icon-us" />;
     } else {
       React.null;
     }}
    <div> "ENV_VAR: "->React.string Env.envVar->React.string </div>
    <div> "GLOBAL_VAR: "->React.string Env.globalVar->React.string </div>
    <PerPageGlobals />
  </div>;
};
