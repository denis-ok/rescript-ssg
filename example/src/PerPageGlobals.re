[@val] external globalThis: Js.Dict.t(Js.Json.t) = "globalThis";

let renderJsonOption = (json: option(Js.Json.t)) =>
  switch (json) {
  | None => "NONE"->React.string
  | Some(json) =>
    Js.Json.stringifyAny(json)
    ->(Belt.Option.getWithDefault(""))
    ->React.string
  };

[@react.component]
let make = () =>
  <div>
    <div>
      "PER_PAGE_GLOBAL_1:"->React.string
      {globalThis->(Js.Dict.get("PER_PAGE_GLOBAL_1"))->renderJsonOption}
    </div>
    <div>
      "PER_PAGE_GLOBAL_2:"->React.string
      {globalThis->(Js.Dict.get("PER_PAGE_GLOBAL_2"))->renderJsonOption}
    </div>
  </div>;
