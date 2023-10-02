let currentDir = Utils.getDirname();

let () =
  Commands.start(
    ~pages=Pages.pages,
    ~devServerOptions={listenTo: Port(9007), proxy: None},
    ~mode=Development,
    ~outputDir=Pages.outputDir,
    ~projectRootDir=Path.join2(currentDir, "../../../"),
    ~logLevel=Info,
    ~globalEnvValues=Pages.globalEnvValues,
    ~webpackBundleAnalyzerMode=None,
    ~buildWorkersCount=1,
    ~esbuildProxyRules=[|
      {
        from: "/v1/",
        to_: {
          target: UnixSocket("foo.sock"),
          pathRewrite: Some({rewriteFrom: "/v1/", rewriteTo: "/api/v1"}),
        },
      },
      {
        from: "/v2/",
        to_: {
          target: Host("http://localhost:1234"),
          pathRewrite: Some({rewriteFrom: "/v2/", rewriteTo: "/api/v2"}),
        },
      },
      {
        from: "/v3/",
        to_: {
          target: Host("http://lala.com"),
          pathRewrite: Some({rewriteFrom: "/v3/", rewriteTo: "/api/v3"}),
        },
      },
    |],
    (),
  );
