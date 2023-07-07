let modulePath = Utils.getFilepath()

@react.component
let make = () => {
  <>
    <MetaTags title="PageWithoutHydration" description="PageWithoutHydration description" />
    <Header h1Text="PageWithoutHydration" />
    <Content />
    <Footer />
  </>
}
