let modulePath = Ssg.Utils.getFilepath();

module PartialHydration = Ssg.PartialHydration;

module Local = {
  [@react.component]
  let make = () => <div> "THIS_SHOULD_BE_HYDRATED"->React.string </div>;
};

[@react.component]
let make = () =>
  <>
    <MetaTags
      title="PageWithPartialHydration"
      description="PageWithPartialHydration description"
    />
    <Header h1Text="PageWithPartialHydration" />
    <div> "THIS_SHOULD_NOT_BE_HYDRATED"->React.string </div>
    <PartialHydration.WithHydration
      moduleName="PageWithPartialHydration.Local">
      <Local />
    </PartialHydration.WithHydration>
    <PartialHydration.WithHydration moduleName="Content">
      <Content />
    </PartialHydration.WithHydration>
    <Footer />
  </>;
