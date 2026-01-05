open NodeTest;

let ( let* ) = (p, f) => Js.Promise.then_(f, p);

NodeTest.Promise.testWithOptions(
  "NodeTest.Promise.testWithOptions, top level test",
  NodeTest.makeOptions(),
  context => {
    context
    |> TestContext.before(_context => {
         Js.log("before");
         Js.Promise.resolve();
       });

    context
    |> TestContext.beforeEach(_context => {
         Js.log("beforeEach");
         Js.Promise.resolve();
       });

    context
    |> TestContext.after(_context => {
         Js.log("after");
         Js.Promise.resolve();
       });

    context
    |> TestContext.afterEach(_context => {
         Js.log("afterEach");
         Js.Promise.resolve();
       });

    context
    |> TestContext.test("TestContext.test 1", _context => {
         Js.log("Running TestContext.test 1");
         expect |> ok(true);
       });

    context
    |> TestContext.test("TestContext.test 2", _context => {
         Js.log("Running TestContext.test 2");
         expect |> ok(true);
       });

    context
    |> TestContext.testWithOptions(
         "TestContext.testWithOptions 1",
         NodeTest.makeOptions(),
         _context => {
           Js.log("Running TestContext.testWithOptions 1");
           expect |> ok(true);
         },
       );

    context
    |> TestContext.testWithOptions(
         "TestContext.testWithOptions 2",
         NodeTest.makeOptions(),
         _context => {
           Js.log("Running TestContext.testWithOptions 2");
           expect |> ok(true);
         },
       );

    // In this example we have to run async subtests one after another to ensure that both subtests have completed.
    // This is necessary because tests do not wait for their subtests to complete, unlike tests created within suites.
    // Any subtests that are still outstanding when their parent finishes are cancelled and treated as failures.
    // Any subtest failures cause the parent test to fail.
    // This is from docs: https://nodejs.org/api/test.html#subtests

    let* () =
      context
      |> TestContext.testPromise("TestContext.testPromise 1", _context => {
           Js.log("Running TestContext.testPromise 1");
           expect |> ok(true);
           Js.Promise.resolve();
         });

    let* () =
      context
      |> TestContext.testPromise("TestContext.testPromise 2", _context => {
           Js.log("Running TestContext.testPromise 2");
           expect |> ok(true);
           Js.Promise.resolve();
         });

    let* () =
      context
      |> TestContext.testPromiseWithOptions(
           "TestContext.testPromiseWithOptions 1",
           NodeTest.makeOptions(),
           _context => {
             Js.log("Running TestContext.testPromiseWithOptions 1");
             expect |> ok(true);
             Js.Promise.resolve();
           },
         );

    let* () =
      context
      |> TestContext.testPromiseWithOptions(
           "TestContext.testPromiseWithOptions 2",
           NodeTest.makeOptions(),
           _context => {
             Js.log("Running TestContext.testPromiseWithOptions 2");
             expect |> ok(true);
             Js.Promise.resolve();
           },
         );

    Js.Promise.resolve();
  },
)
|> Js.Promise.then_(() => {
     Js.log("Test completed!");
     Js.Promise.resolve();
   })
|> ignore;
