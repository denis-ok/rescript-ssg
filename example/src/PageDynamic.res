let modulePath = Utils.getFilepath()

@react.component
let make = () => {
  let url = ReasonReactRouter.useUrl(
    ~serverUrl={path: list{"page-without-data", "server-id"}, hash: "", search: ""},
    (),
  )

  let path =
    url.path->Belt.List.reverse->Belt.List.head->Belt.Option.getWithDefault("None(unexpected)")

  <>
    <MetaTags title="PageDynamic" description="PageDynamic description" />
    <Header h1Text="PageDynamic" />
    <h2> {"Dynamic path part: "->React.string} {path->React.string} </h2>
    <Content />
    <Footer />
  </>
}
