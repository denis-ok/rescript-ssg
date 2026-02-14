let modulePath = RescriptSsg.Utils.getFilepath();

module Local = {
  [@react.component]
  let make = () => <div> "THIS_SHOULD_BE_HYDRATED"->React.string </div>;
};

[@react.component]
let make = () =>
  <>
    <MetaTags title="PageWithPartialHydration" description="PageWithPartialHydration description" />
    <Header h1Text="PageWithPartialHydration" />
    <div> "THIS_SHOULD_NOT_BE_HYDRATED"->React.string </div>
    <RescriptSsg.PartialHydration.WithHydration moduleName="PageWithPartialHydration.Local">
      <Local />
    </RescriptSsg.PartialHydration.WithHydration>
    <RescriptSsg.PartialHydration.WithHydration moduleName="Content">
      <Content />
    </RescriptSsg.PartialHydration.WithHydration>
    <Footer />
  </>;
