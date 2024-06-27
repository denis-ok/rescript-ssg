module Server = {
  type t;
  [@send] external listen: (t, int, unit => unit) => unit = "listen";

  [@send] external close: (t, unit => unit) => unit = "close";

  [@send]
  external closeAllConnections: (t, unit) => unit = "closeAllConnections";

  [@set] external setKeepAliveTimeoutMs: (t, int) => unit = "keepAliveTimeout";
};

module ClientRequest = {
  // https://nodejs.org/api/http.html#class-httpclientrequest
  type t;

  type error;

  [@send] external on: (t, string, error => unit) => unit = "on";
};

// https://nodejs.org/api/http.html#httprequestoptions-callback
type nodeRequestOptions = {
  hostname: option(string),
  port: option(int),
  path: string,
  method: string,
  headers: Js.Dict.t(string),
  // Cannot be used if one of host or port is specified, as those specify a TCP Socket.
  socketPath: option(string),
};

module ServerResponse = {
  type t;
  [@get] external statusCode: t => int = "statusCode";

  [@send]
  external writeHead:
    (t, ~statusCode: int, ~headers: option(Js.Dict.t(string))) => t =
    "writeHead";

  [@mel.send] external setHeader: (t, string, string) => unit = "setHeader";

  [@send] external end_: (t, string) => unit = "end";
};

module IncommingMessage = {
  type t;
  [@get] external statusCode: t => int = "statusCode";
  [@get] external url: t => string = "url";
  [@get] external method: t => string = "method";
  [@get] external headers: t => Js.Dict.t(string) = "headers";

  type pipeOptions = {
    [@bs.as "end"]
    end_: bool,
  };

  [@send]
  external pipeToServerResponse: (t, ServerResponse.t, pipeOptions) => unit =
    "pipe";
  [@send]
  external pipeToClientRequest: (t, ClientRequest.t, pipeOptions) => unit =
    "pipe";
};

[@bs.module "node:http"]
external nodeCreateServer:
  ((IncommingMessage.t, ServerResponse.t) => unit) => Server.t =
  "createServer";

[@bs.module "node:https"]
external nodeHttpsCreateServer:
  ((IncommingMessage.t, ServerResponse.t) => unit) => Server.t =
  "createServer";

[@bs.module "node:http"]
external nodeRequest:
  (nodeRequestOptions, IncommingMessage.t => unit) => ClientRequest.t =
  "request";

[@bs.module "node:https"]
external nodeHttpsRequest:
  (nodeRequestOptions, IncommingMessage.t => unit) => ClientRequest.t =
  "request";

module Url = {
  type t;
  [@bs.new] [@bs.scope "global"]
  external makeExn: (string, ~base: option(string)) => t = "URL";
  [@bs.get] external hash: t => string = "hash";
  [@bs.get] external host: t => string = "host";
  [@bs.get] external hostname: t => string = "hostname";
  [@bs.get] external href: t => string = "href";
  [@bs.get] external origin: t => string = "origin";
  [@bs.get] external protocol: t => string = "protocol";
  [@bs.get] external pathname: t => string = "pathname";
  // Yes, port parsed as string
  // https://nodejs.org/api/url.html#urlport
  [@bs.get] external port: t => string = "port";
  [@bs.get] external search: t => string = "search";
  [@bs.get] external searchParams: t => Js.Dict.t(string) = "searchParams";

  let make = (path, ~base) =>
    switch (makeExn(path, ~base)) {
    | url => Some(url)
    | exception _ => None
    };
};

module ProxyRule = {
  type target =
    | Url(string)
    | UnixSocket(string);

  type pathRewrite = {
    pathRewriteFrom: string,
    pathRewriteTo: string,
  };

  type proxyTo = {
    target,
    pathRewrite: option(pathRewrite),
    changeOrigin: bool,
    secure: bool,
  };

  type t = {
    fromPath: string,
    proxyTo,
  };
};

module ValidProxyRule = {
  type target =
    | Url(Url.t)
    | UnixSocket(string);

  type proxyTo = {
    target,
    pathRewrite: option(ProxyRule.pathRewrite),
    changeOrigin: bool,
    secure: bool,
  };

  type t = {
    fromPath: string,
    proxyTo,
    changeOrigin: bool,
    secure: bool,
  };

  let fromProxyRule = (proxyRule: ProxyRule.t): t => {
    let target =
      switch (proxyRule.proxyTo.target) {
      | UnixSocket(path) => UnixSocket(path)
      | Url(str) =>
        let url = Url.make(str, ~base=None);
        switch (url) {
        | None =>
          Js.Console.error2(
            "[Dev server] Error, failed to parse URL string:",
            str,
          );
          Process.exit(1);
        | Some(url) => Url(url)
        };
      };

    {
      fromPath: proxyRule.fromPath,
      proxyTo: {
        target,
        pathRewrite: proxyRule.proxyTo.pathRewrite,
        changeOrigin: proxyRule.proxyTo.changeOrigin,
        secure: proxyRule.proxyTo.secure,
      },
      secure: proxyRule.proxyTo.secure,
      changeOrigin: proxyRule.proxyTo.changeOrigin,
    };
  };
};

let sortPathsBySegmentCount = (a, b) => {
  // Sort paths to make sure that more specific rules are matched first.
  let countSegments = s =>
    s
    ->Js.String2.split("/")
    ->Js.Array2.filter(s => s != "")
    ->Js.Array2.length;

  let segCount1 = countSegments(a);
  let segCount2 = countSegments(b);

  if (segCount1 == segCount2) {
    0;
  } else if (segCount1 < segCount2) {
    1;
  } else {
    (-1);
  };
};

let isPageWithDynamicPathSegmentRequested =
    (reqPath: string, pagePath: string) => {
  let makeSegments = path =>
    path
    ->Utils.maybeAddSlashPrefix
    ->Utils.maybeAddSlashSuffix
    ->Js.String2.split("/")
    ->Js.Array2.filter(s => s != "")
    ->Belt.List.fromArray;

  let reqPathSegments = reqPath->makeSegments;
  let pagePathSegments = pagePath->makeSegments;

  let rec isMatch = (reqPathSegments, pagePathSegments) => {
    switch (reqPathSegments, pagePathSegments) {
    | ([], []) => true
    | ([], _)
    | (_, []) => false
    | ([reqSegment, ...reqTail], [pageSegment, ...pageTail]) =>
      pageSegment == PagePath.dynamicSegment || reqSegment == pageSegment
        ? isMatch(reqTail, pageTail) : false
    };
  };
  isMatch(reqPathSegments, pagePathSegments);
};

let start =
    (
      ~port: int,
      ~targetHost: string,
      ~targetPort: int,
      ~proxyRules: array(ProxyRule.t),
      ~pagePaths: array(string),
    ) => {
  let pagePathsWithDynamicSegments =
    pagePaths
    ->Js.Array2.filter(path =>
        path->Js.String2.includes(PagePath.dynamicSegment)
      )
    ->Js.Array2.sortInPlaceWith(sortPathsBySegmentCount);

  let proxyRules =
    proxyRules
    ->Js.Array2.map(rule => ValidProxyRule.fromProxyRule(rule))
    ->Js.Array2.sortInPlaceWith((a, b) =>
        sortPathsBySegmentCount(a.fromPath, b.fromPath)
      );

  let server =
    nodeCreateServer((req, res) => {
      let reqUrl = req->IncommingMessage.url;
      let reqHeaders = req->IncommingMessage.headers;
      let reqHost =
        reqHeaders->Js.Dict.get("host")->Belt.Option.getWithDefault("");
      let urlBase = "http://" ++ reqHost;
      let url = Url.make(reqUrl, ~base=Some(urlBase));

      let (reqPath, reqQueryString) =
        switch (url) {
        | None => (reqUrl, "")
        | Some(url) => (url->Url.pathname, url->Url.search)
        };

      let targetReqOptions: nodeRequestOptions = {
        let defaultTarget = {
          hostname: Some(targetHost),
          port: Some(targetPort),
          path: req->IncommingMessage.url,
          method: req->IncommingMessage.method,
          headers: req->IncommingMessage.headers,
          socketPath: None,
        };

        let reqPathNormalized =
          reqPath->Utils.maybeAddSlashPrefix->Utils.maybeAddSlashSuffix;

        let exactPagePathRelatedToRequestedPath = {
          pagePaths->Js.Array2.find(pagePath => pagePath == reqPathNormalized);
        };

        switch (exactPagePathRelatedToRequestedPath) {
        | Some(_) =>
          // esbuild server redirects request to a path with trailing slash if a path without trailing slash requested.
          // To avoid this redirect we add trailing slash.
          {
            ...defaultTarget,
            path: defaultTarget.path->Utils.maybeAddSlashSuffix,
          }
        | None =>
          let relatedPagePathWithDynamicSegment =
            pagePathsWithDynamicSegments->Js.Array2.find(pagePath =>
              isPageWithDynamicPathSegmentRequested(reqPath, pagePath)
            );
          switch (relatedPagePathWithDynamicSegment) {
          | Some(relatedPagePathWithDynamicSegment) =>
            Js.log2(
              "[Dev server] Page with dynamic segment requested, path rewritten to:",
              relatedPagePathWithDynamicSegment,
            );
            {
              hostname: Some(targetHost),
              port: Some(targetPort),
              path: relatedPagePathWithDynamicSegment ++ reqQueryString,
              method: req->IncommingMessage.method,
              headers: req->IncommingMessage.headers,
              socketPath: None,
            };
          | None =>
            let matchedProxyRule =
              proxyRules->Js.Array2.find(rule =>
                reqPath->Js.String2.startsWith(rule.fromPath)
              );
            switch (matchedProxyRule) {
            | None =>
              // Technically, this is some kind of error:
              // User requested a path that doesn't have related page and proxy rule for this request also not exist.
              defaultTarget
            | Some({
                fromPath,
                proxyTo: {target, pathRewrite, changeOrigin, secure},
                _,
              }) =>
              let (path, isPathRewritten) =
                switch (pathRewrite) {
                | None => (reqPath, false)
                | Some({pathRewriteFrom, pathRewriteTo}) =>
                  let newPath =
                    reqPath->Js.String2.replace(
                      pathRewriteFrom,
                      pathRewriteTo,
                    );
                  (newPath, true);
                };

              switch (isPathRewritten) {
              | false => Js.log2("[Dev server] Proxy rule matched:", fromPath)
              | true =>
                Js.log(
                  {j|[Dev server] Proxy rule matched: $(fromPath), path rewritten to: $(path)|j},
                )
              };

              switch (target) {
              | UnixSocket(socketPath) => {
                  hostname: None,
                  port: None,
                  socketPath: Some(socketPath),
                  path: path ++ reqQueryString,
                  method: req->IncommingMessage.method,
                  headers: req->IncommingMessage.headers,
                }
              | Url(url) =>
                let hostName = url->Url.hostname;
                if (changeOrigin) {
                  Js.Dict.set(
                    req->IncommingMessage.headers,
                    "host",
                    hostName,
                  );
                };

                {
                  hostname: Some(hostName),
                  port:
                    Some(
                      url
                      ->Url.port
                      ->Belt.Int.fromString
                      ->Belt.Option.getWithDefault(secure ? 443 : 80),
                    ),
                  socketPath: None,
                  path: path ++ reqQueryString,
                  method: req->IncommingMessage.method,
                  headers: req->IncommingMessage.headers,
                };
              };
            };
          };
        };
      };

      let proxyReq = {
        let writeHeaders = targetRes => {
          res
          ->ServerResponse.writeHead(
              ~statusCode=targetRes->IncommingMessage.statusCode,
              ~headers=Some(targetRes->IncommingMessage.headers),
            )
          ->ignore;
          targetRes->IncommingMessage.pipeToServerResponse(
            res,
            {end_: true},
          );
        };

        switch (targetReqOptions.port) {
        | Some(443) => nodeHttpsRequest(targetReqOptions, writeHeaders)
        | _ => nodeRequest(targetReqOptions, writeHeaders)
        };
      };
      proxyReq->ClientRequest.on("error", error => {
        Js.Console.error2("[Dev server] Error with proxy request:", error);
        res
        ->ServerResponse.writeHead(~statusCode=404, ~headers=None)
        ->ServerResponse.end_("[Dev server] Internal server error");
      });

      req->IncommingMessage.pipeToClientRequest(proxyReq, {end_: true});
    });

  server->Server.setKeepAliveTimeoutMs(2000);

  let startServer = () =>
    server->Server.listen(port, () =>
      Js.log("[Dev server] Listening on port " ++ string_of_int(port))
    );

  switch (startServer()) {
  | () => ()
  | exception exn =>
    Js.Console.error2("[Dev server] Failed to start, error:", exn);
    Process.exit(1);
  };

  GracefulShutdown.addTask(() => {
    Js.log("[Dev server] Stopping dev server...");

    Promise.make((~resolve, ~reject as _reject) => {
      let unit = ();

      server->Server.close(() => {
        Js.log("[Dev server] Stopped successfully");
        resolve(. unit);
      });

      server->Server.closeAllConnections();

      Js.Global.setTimeout(
        () => {
          Js.Console.error("[Dev server] Failed to gracefully shutdown.");
          Process.exit(1);
        },
        GracefulShutdown.gracefulShutdownTimeout,
      )
      ->ignore;
    });
  });

  ();
};
