@val external globalThis: Js.Dict.t<string> = "globalThis"

let value = globalThis->Js.Dict.get("PER_PAGE_GLOBAL")

@react.component
let make = () => {
  <div>
    {"PER_PAGE_GLOBAL: "->React.string} {value->Belt.Option.getWithDefault("")->React.string}
  </div>
}
