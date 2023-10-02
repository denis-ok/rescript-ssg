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
        from: "/v4/",
        to_: {
          target: {
            host: None,
            port: None,
            unixSocket: Some("foo.sock"),
          },
          pathRewrite: Some({rewriteFrom: "/v4/", rewriteTo: "/api/"}),
        },
      },
    |],
    (),
  );
