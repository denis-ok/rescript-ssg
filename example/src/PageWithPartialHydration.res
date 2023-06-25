let modulePath = Utils.getFilepath()

module Local = {
  @react.component
  let make = () => {
    <div> {"Local"->React.string} {"THIS_SHOULD_BE_HYDRATED"->React.string} </div>
  }
}

@react.component
let make = () => {
  <div>
    {"THIS_SHOULD_NOT_BE_HYDRATED"->React.string}
    <SharedModule.Header />
    <PartialHydration.WithHydration moduleName="SharedModule.Footer">
      <SharedModule.Footer />
    </PartialHydration.WithHydration>
    <PartialHydration.WithHydration moduleName="PageWithPartialHydration.Local">
      <Local />
    </PartialHydration.WithHydration>
  </div>
}
