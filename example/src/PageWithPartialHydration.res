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
    <WithHydration moduleName="SharedModule.Footer"> <SharedModule.Footer /> </WithHydration>
    <WithHydration moduleName="PageWithPartialHydration.Local"> <Local /> </WithHydration>
  </div>
}
