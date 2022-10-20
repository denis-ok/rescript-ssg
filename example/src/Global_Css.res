open CssJs

let injectGlobal = () =>
  global(.
    "body",
    [
      //
      fontSize(#px(14)),
      backgroundColor(turquoise),
    ],
  )
