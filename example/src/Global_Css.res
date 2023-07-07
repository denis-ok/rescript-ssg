open CssJs

let injectGlobal = () => {
  global(.
    "body",
    [
      //
      fontSize(#px(16)),
      backgroundColor(#hex("FADCD9")),
      padding(#px(32)),
    ],
  )
  global(.
    "h1,h2,h3,h4,h5,h6",
    [
      //
      margin(#zero),
      fontWeight(#normal),
    ],
  )
  global(.
    "a",
    [
      //
      color(black),
    ],
  )
  global(.
    "a:hover",
    [
      //
      color(violet),
    ],
  )
}
