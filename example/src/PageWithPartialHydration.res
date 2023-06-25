let modulePath = Utils.getFilepath()

module Local = {
  @react.component
  let make = () => {
    <div> {"Local"->React.string} </div>
  }
}

@react.component
let make = () => {
  <div>
    <SharedModule.Header />
    <PartialHydration.WithHydration moduleName="SharedModule.Footer">
      <SharedModule.Footer />
    </PartialHydration.WithHydration>
    <PartialHydration.WithHydration moduleName="PageWithPartialHydration.Local">
      <Local />
    </PartialHydration.WithHydration>
  </div>
}
