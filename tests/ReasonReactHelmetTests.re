open NodeTest;
open RescriptSsg;

let ( let* ) = (p, f) => Js.Promise.then_(f, p);

NodeTest.Promise.testWithOptions(
  "ReasonReactHelmet",
  NodeTest.makeOptions(),
  context => {
    context
    |> TestContext.beforeEach(_context => {
         ReasonReactHelmet.resetState();
         Js.Promise.resolve();
       });

    // Test: Empty state returns empty strings
    let* () =
      context
      |> TestContext.test("empty state returns empty strings", _context => {
           let helmet = ReasonReactHelmet.renderStatic();

           expect |> equal(helmet.title.toString(), "");
           expect |> equal(helmet.meta.toString(), "");
           expect |> equal(helmet.htmlAttributes.toString(), "");
           expect |> equal(helmet.bodyAttributes.toString(), "");
           expect |> equal(helmet.link.toString(), "");
           expect |> equal(helmet.script.toString(), "");
           expect |> equal(helmet.noscript.toString(), "");
           expect |> equal(helmet.style.toString(), "");
           expect |> equal(helmet.base.toString(), "");

           Js.Promise.resolve();
         });

    // Test: Title rendering
    let* () =
      context
      |> TestContext.test("renders title", _context => {
           let _ =
             ReactDOMServer.renderToString(
               <ReasonReactHelmet> <title> "Hello World"->React.string </title> </ReasonReactHelmet>,
             );

           let helmet = ReasonReactHelmet.renderStatic();
           expect |> equal(helmet.title.toString(), "<title>Hello World</title>");

           Js.Promise.resolve();
         });

    // Test: Title with special characters
    let* () =
      context
      |> TestContext.test("escapes special characters in title", _context => {
           let _ =
             ReactDOMServer.renderToString(
               <ReasonReactHelmet> <title> "Test <script> & \"quotes\""->React.string </title> </ReasonReactHelmet>,
             );

           let helmet = ReasonReactHelmet.renderStatic();
           expect |> equal(helmet.title.toString(), "<title>Test &lt;script&gt; &amp; &quot;quotes&quot;</title>");

           Js.Promise.resolve();
         });

    // Test: Meta tags
    let* () =
      context
      |> TestContext.test("renders meta tags", _context => {
           let _ =
             ReactDOMServer.renderToString(
               <ReasonReactHelmet> <meta name="description" content="Test description" /> </ReasonReactHelmet>,
             );

           let helmet = ReasonReactHelmet.renderStatic();
           let metaStr = helmet.meta.toString();
           expect |> equal(metaStr, {|<meta content="Test description" name="description"/>|});

           Js.Promise.resolve();
         });

    // Test: Multiple meta tags
    let* () =
      context
      |> TestContext.test("renders multiple meta tags", _context => {
           let _ =
             ReactDOMServer.renderToString(
               <ReasonReactHelmet>
                 <meta name="description" content="Test description" />
                 <meta property="og:title" content="OG Title" />
               </ReasonReactHelmet>,
             );

           let helmet = ReasonReactHelmet.renderStatic();
           let metaStr = helmet.meta.toString();
           // Both meta tags should be present
           expect
           |> equal(
                metaStr,
                {|<meta content="OG Title" property="og:title"/>
<meta content="Test description" name="description"/>|},
              );

           Js.Promise.resolve();
         });

    // Test: HTML attributes
    let* () =
      context
      |> TestContext.test("renders html attributes", _context => {
           let _ = ReactDOMServer.renderToString(<ReasonReactHelmet> <html lang="en" /> </ReasonReactHelmet>);

           let helmet = ReasonReactHelmet.renderStatic();
           expect |> equal(helmet.htmlAttributes.toString(), {|lang="en"|});

           Js.Promise.resolve();
         });

    // Test: Multiple HTML attributes
    let* () =
      context
      |> TestContext.test("renders multiple html attributes", _context => {
           let _ =
             ReactDOMServer.renderToString(<ReasonReactHelmet> <html lang="en" dir="ltr" /> </ReasonReactHelmet>);

           let helmet = ReasonReactHelmet.renderStatic();
           let htmlAttrs = helmet.htmlAttributes.toString();
           expect |> equal(htmlAttrs, {|dir="ltr" lang="en"|});

           Js.Promise.resolve();
         });

    // Test: Body attributes
    let* () =
      context
      |> TestContext.test("renders body attributes", _context => {
           let _ =
             ReactDOMServer.renderToString(<ReasonReactHelmet> <body className="dark-mode" /> </ReasonReactHelmet>);

           let helmet = ReasonReactHelmet.renderStatic();
           expect |> equal(helmet.bodyAttributes.toString(), {|className="dark-mode"|});

           Js.Promise.resolve();
         });

    // Test: Link tags
    let* () =
      context
      |> TestContext.test("renders link tags", _context => {
           let _ =
             ReactDOMServer.renderToString(
               <ReasonReactHelmet> <link rel="stylesheet" href="/styles.css" /> </ReasonReactHelmet>,
             );

           let helmet = ReasonReactHelmet.renderStatic();
           let linkStr = helmet.link.toString();
           expect |> equal(linkStr, {|<link href="/styles.css" rel="stylesheet"/>|});

           Js.Promise.resolve();
         });

    // Test: Canonical link
    let* () =
      context
      |> TestContext.test("renders canonical link", _context => {
           let _ =
             ReactDOMServer.renderToString(
               <ReasonReactHelmet> <link rel="canonical" href="https://example.com/page" /> </ReasonReactHelmet>,
             );

           let helmet = ReasonReactHelmet.renderStatic();
           let linkStr = helmet.link.toString();
           expect |> equal(linkStr, {|<link href="https://example.com/page" rel="canonical"/>|});

           Js.Promise.resolve();
         });

    // Test: Script tags with src
    let* () =
      context
      |> TestContext.test("renders script tags with src", _context => {
           let _ =
             ReactDOMServer.renderToString(
               <ReasonReactHelmet> <script src="/app.js" async=true /> </ReasonReactHelmet>,
             );

           let helmet = ReasonReactHelmet.renderStatic();
           let scriptStr = helmet.script.toString();

           expect |> equal(scriptStr, {|<script async="true" src="/app.js"></script>|});

           Js.Promise.resolve();
         });

    // Test: Script tags with inline content
    let* () =
      context
      |> TestContext.test("renders script tags with inline content", _context => {
           let _ =
             ReactDOMServer.renderToString(
               <ReasonReactHelmet>
                 <script type_="application/ld+json"> {|{"@context": "https://schema.org"}|}->React.string </script>
               </ReasonReactHelmet>,
             );

           let helmet = ReasonReactHelmet.renderStatic();
           let scriptStr = helmet.script.toString();
           expect
           |> equal(scriptStr, {|<script type="application/ld+json">{"@context": "https://schema.org"}</script>|});

           Js.Promise.resolve();
         });

    // Test: Empty style tag
    let* () =
      context
      |> TestContext.test("renders style tags", _context => {
           let _ = ReactDOMServer.renderToString(<ReasonReactHelmet> <style type_="text/css" /> </ReasonReactHelmet>);

           let helmet = ReasonReactHelmet.renderStatic();
           let styleStr = helmet.style.toString();
           expect |> equal(styleStr, {|<style type="text/css"></style>|});

           Js.Promise.resolve();
         });

    // Test: Style tags with CSS content
    let* () =
      context
      |> TestContext.test("renders style tags with css content", _context => {
           let _ =
             ReactDOMServer.renderToString(
               <ReasonReactHelmet>
                 <style type_="text/css"> "body { background: red; }"->React.string </style>
               </ReasonReactHelmet>,
             );

           let helmet = ReasonReactHelmet.renderStatic();
           let styleStr = helmet.style.toString();
           expect |> equal(styleStr, {|<style type="text/css">body { background: red; }</style>|});

           Js.Promise.resolve();
         });

    // Test: Base tag
    let* () =
      context
      |> TestContext.test("renders base tag", _context => {
           let _ =
             ReactDOMServer.renderToString(
               <ReasonReactHelmet> <base href="https://example.com/" /> </ReasonReactHelmet>,
             );

           let helmet = ReasonReactHelmet.renderStatic();
           expect |> equal(helmet.base.toString(), {|<base href="https://example.com/"/>|});

           Js.Promise.resolve();
         });

    // Test: Empty noscript tag
    let* () =
      context
      |> TestContext.test("renders noscript tags", _context => {
           let _ = ReactDOMServer.renderToString(<ReasonReactHelmet> <noscript /> </ReasonReactHelmet>);

           let helmet = ReasonReactHelmet.renderStatic();
           let noscriptStr = helmet.noscript.toString();
           expect |> equal(noscriptStr, {|<noscript></noscript>|});

           Js.Promise.resolve();
         });

    // Test: Noscript tags with content
    let* () =
      context
      |> TestContext.test("renders noscript tags with content", _context => {
           let _ =
             ReactDOMServer.renderToString(
               <ReasonReactHelmet>
                 <noscript> "Please enable JavaScript"->React.string </noscript>
               </ReasonReactHelmet>,
             );

           let helmet = ReasonReactHelmet.renderStatic();
           let noscriptStr = helmet.noscript.toString();
           expect |> equal(noscriptStr, {|<noscript>Please enable JavaScript</noscript>|});

           Js.Promise.resolve();
         });

    // Test: State resets after renderStatic
    let* () =
      context
      |> TestContext.test("state resets after renderStatic", _context => {
           let _ =
             ReactDOMServer.renderToString(
               <ReasonReactHelmet> <title> "First"->React.string </title> </ReasonReactHelmet>,
             );

           let helmet1 = ReasonReactHelmet.renderStatic();
           expect |> equal(helmet1.title.toString(), "<title>First</title>");

           // After renderStatic, state should be empty
           let helmet2 = ReasonReactHelmet.renderStatic();
           expect |> equal(helmet2.title.toString(), "");

           Js.Promise.resolve();
         });

    // Test: Full example matching MetaTags.re usage
    let* () =
      context
      |> TestContext.test("full example with html, title, and meta", _context => {
           let _ =
             ReactDOMServer.renderToString(
               <ReasonReactHelmet>
                 <html lang="en" />
                 <title> "My Page Title"->React.string </title>
                 <meta name="description" content="Page description" />
               </ReasonReactHelmet>,
             );

           let helmet = ReasonReactHelmet.renderStatic();

           expect |> equal(helmet.htmlAttributes.toString(), {|lang="en"|});
           expect |> equal(helmet.title.toString(), "<title>My Page Title</title>");
           expect |> equal(helmet.meta.toString(), {|<meta content="Page description" name="description"/>|});

           Js.Promise.resolve();
         });

    // Test: Multiple Helmet components (nested scenario - last one wins)
    let* () =
      context
      |> TestContext.test("multiple helmet components accumulate state", _context => {
           let _ =
             ReactDOMServer.renderToString(
               <div>
                 <ReasonReactHelmet>
                   <title> "First Title"->React.string </title>
                   <meta name="author" content="Author 1" />
                 </ReasonReactHelmet>
                 <ReasonReactHelmet>
                   <title> "Second Title"->React.string </title>
                   <meta name="keywords" content="test, react" />
                 </ReasonReactHelmet>
               </div>,
             );

           let helmet = ReasonReactHelmet.renderStatic();

           // The second title should override the first
           expect |> equal(helmet.title.toString(), "<title>Second Title</title>");

           // Both meta tags should be present
           expect
           |> equal(
                helmet.meta.toString(),
                {|<meta content="test, react" name="keywords"/>
<meta content="Author 1" name="author"/>|},
              );

           Js.Promise.resolve();
         });

    Js.Promise.resolve();
  },
)
|> ignore;
