let modulePath = Utils.getFilepath()

@react.component
let make = () => <>
  <MetaTags title="PageWithoutData" description="PageWithoutData description" />
  <Header h1Text="PageWithoutData" />
  <Content />
  <Footer />
</>
