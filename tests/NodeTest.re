/** Bindings for functions from {{: https://nodejs.org/api/test.html#test-runner }node:test} and {{: https://nodejs.org/api/assert.html#strict-assertion-mode }node:assert/strict} modules  */;

/** Abstract type for {{: https://developer.mozilla.org/en-US/docs/Web/API/AbortSignal }AbortSignal} */
type abortSignal;

/** Options for configuring test behavior. See {{: https://nodejs.org/api/test.html#testname-options-fn }node:test options} */
[@deriving jsProperties]
type testOptions = {
  /** If a number is provided, then that many tests would run in parallel within the application thread */
  [@mel.optional]
  concurrency: option(int),
  /** If truthy, and the test context is configured to run only tests, then this test will be run */
  [@mel.optional]
  only: option(bool),
  /** Allows aborting an in-progress test */
  [@mel.optional]
  signal: option(abortSignal),
  /** If truthy, the test is skipped */
  [@mel.optional]
  skip: option(bool),
  /** If truthy, the test is marked as TODO */
  [@mel.optional]
  todo: option(bool),
  /** A number of milliseconds the test will fail after */
  [@mel.optional]
  timeout: option(int),
  /** The number of assertions and subtests expected to be run in the test */
  [@mel.optional]
  plan: option(int),
};

module TestContext = {
  /** https://nodejs.org/api/test.html#class-testcontext */
  type t;

  // This function is used to create a hook running before subtest of the current test.
  [@mel.send] external before: (t => Js.Promise.t(unit), [@mel.this] t) => unit = "before";
  // This function is used to create a hook that runs after the current test finishes.
  [@mel.send] external after: (t => Js.Promise.t(unit), [@mel.this] t) => unit = "after";

  // This function is used to create a hook running before each subtest of the current test.
  [@mel.send] external beforeEach: (t => Js.Promise.t(unit), [@mel.this] t) => unit = "beforeEach";
  // This function is used to create a hook running after each subtest of the current test.
  [@mel.send] external afterEach: (t => Js.Promise.t(unit), [@mel.this] t) => unit = "afterEach";

  [@mel.send] external test: (string, t => Js.Promise.t(unit), [@mel.this] t) => Js.Promise.t(unit) = "test";

  [@mel.send]
  external testWithOptions: (string, testOptions, t => Js.Promise.t(unit), [@mel.this] t) => Js.Promise.t(unit) =
    "test";
};

/** Create a test with a given name and callback function that runs the test */
[@mel.module "node:test"]
external test: (string, TestContext.t => unit) => unit = "test";

let makeOptions = testOptions;

/** Create a test with a given name, options, and callback function that runs the test */
[@mel.module "node:test"]
external testWithOptions: (string, testOptions, TestContext.t => unit) => Js.Promise.t(unit) = "test";

/** This promise-based module is needed for nested tests, see {: https://nodejs.org/api/test.html#subtests } */
module Promise = {
  /** {{:https://v2.ocaml.org/manual/bindingops.html }Monadic binding operator} for promises */

  let ( let* ) = (p, f) => Js.Promise.then_(f, p);

  /** Create a top-level test with a given name and callback function that runs the test and returns a promise. */
  [@mel.module "node:test"]
  external test: (string, TestContext.t => Js.Promise.t(unit)) => unit = "test";

  /** Create a subtest with a given name and callback function that runs the test and returns a promise. It is supposed to be used inside a {!test} function call. */
  [@mel.module "node:test"]
  external subtest: (string, unit => Js.Promise.t(unit)) => Js.Promise.t(unit) = "test";

  /** Create a top-level test with options that returns a promise */
  [@mel.module "node:test"]
  external testWithOptions: (string, testOptions, TestContext.t => Js.Promise.t(unit)) => Js.Promise.t(unit) =
    "test";

  /** Create a subtest with options that returns a promise */
  [@mel.module "node:test"]
  external subtestWithOptions: (string, testOptions, unit => Js.Promise.t(unit)) => Js.Promise.t(unit) = "test";
};

/** Abstract type for the {{: https://nodejs.org/api/assert.html#strict-assertion-mode} node:assert/strict} module  */

type assertion;

/** The {{: https://nodejs.org/api/assert.html#strict-assertion-mode} node:assert/strict} module object */
[@mel.module]
external expect: assertion = "node:assert/strict";

/** Tests if the given value is true */
[@mel.send]
external ok: (bool, [@mel.this] assertion) => unit = "ok";

/** Tests strict equality between the actual and expected parameters as determined by {{: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object/is }Object.is()} */
[@mel.send]
external equal: ('a, 'a, [@mel.this] assertion) => unit = "strictEqual";

/** Tests for deep strict equality between the actual and expected parameters */
[@mel.send]
external deep_equal: ('a, 'a, [@mel.this] assertion) => unit = "deepStrictEqual";

/** Alias for {!deep_equal} */

let deepEqual = deep_equal;
