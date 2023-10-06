module Server = {
  type t;
  [@send] external listen: (t, int, unit => unit) => unit = "listen";

  [@send] external close: (t, unit => unit) => unit = "close";
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

[@bs.module "node:http"]
external nodeRequest:
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
    rewriteFrom: string,
    rewriteTo: string,
  };

  type settingTo = {
    target,
    pathRewrite: option(pathRewrite),
  };

  type t = {
    from: string,
    to_: settingTo,
  };
};

module ValidProxyRule = {
  type target =
    | Url(Url.t)
    | UnixSocket(string);

  type settingTo = {
    target,
    pathRewrite: option(ProxyRule.pathRewrite),
  };

  type t = {
    from: string,
    to_: settingTo,
  };

  let fromProxyRule = (proxyRule: ProxyRule.t): t => {
    let target =
      switch (proxyRule.to_.target) {
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
      from: proxyRule.from,
      to_: {
        target,
        pathRewrite: proxyRule.to_.pathRewrite,
      },
    };
  };
};

let dynamicPathSegment = "__dynamic-segment__";

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
  let reqPathSegments = reqPath->Js.String2.split("/")->Belt.List.fromArray;
  let pagePathSegments = pagePath->Js.String2.split("/")->Belt.List.fromArray;

  let rec isMatch = (reqPathSegments, pagePathSegments) => {
    switch (reqPathSegments, pagePathSegments) {
    | ([], []) => true
    | ([], _)
    | (_, []) => false
    | ([reqSegment, ...reqTail], [pageSegment, ...pageTail]) =>
      pageSegment == dynamicPathSegment || reqSegment == pageSegment
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
  let pagePathsWithDynamicSegment =
    pagePaths
    ->Js.Array2.filter(path => path->Js.String2.includes(dynamicPathSegment))
    ->Js.Array2.sortInPlaceWith(sortPathsBySegmentCount);

  let proxyRules =
    proxyRules
    ->Js.Array2.map(rule => ValidProxyRule.fromProxyRule(rule))
    ->Js.Array2.sortInPlaceWith((a, b) =>
        sortPathsBySegmentCount(a.from, b.from)
      );

  let server =
    nodeCreateServer((req, res) => {
      let reqUrl = req->IncommingMessage.url;
      let reqHeaders = req->IncommingMessage.headers;
      let reqHost =
        reqHeaders->Js.Dict.get("host")->Belt.Option.getWithDefault("");
      let urlBase = "http://" ++ reqHost;
      let url = Url.make(reqUrl, ~base=Some(urlBase));
      let reqPath =
        switch (url) {
        | None => reqUrl
        | Some(url) => url->Url.pathname
        };

      let targetOptions: nodeRequestOptions = {
        // Try to match if a page with dynamic segment requested first and then try to match proxy rule.
        let pageWithDynamicPathSegment =
          pagePathsWithDynamicSegment->Js.Array2.find(pagePath =>
            isPageWithDynamicPathSegmentRequested(reqPath, pagePath)
          );
        switch (pageWithDynamicPathSegment) {
        | Some(pagePath) =>
          Js.log2(
            "[Dev server] A page with dynamic segment requested. Rewriting path to:",
            pagePath,
          );
          {
            hostname: Some(targetHost),
            port: Some(targetPort),
            path: pagePath,
            method: req->IncommingMessage.method,
            headers: req->IncommingMessage.headers,
            socketPath: None,
          };
        | None =>
          let matchedRule =
            proxyRules->Js.Array2.find(rule =>
              reqPath->Js.String2.startsWith(rule.from)
            );
          switch (matchedRule) {
          | None => {
              hostname: Some(targetHost),
              port: Some(targetPort),
              path: req->IncommingMessage.url,
              method: req->IncommingMessage.method,
              headers: req->IncommingMessage.headers,
              socketPath: None,
            }
          | Some({from: _, to_: {target, pathRewrite}} as proxyRule) =>
            Js.log2("[Dev server] Proxy rule matched:", proxyRule);
            let path =
              switch (pathRewrite) {
              | Some({rewriteFrom, rewriteTo}) =>
                let newPath =
                  reqPath->Js.String2.replace(rewriteFrom, rewriteTo);
                Js.log2("[Dev server] Path rewritten, new path:", newPath);
                newPath;
              | None => reqPath
              };
            switch (target) {
            | UnixSocket(socketPath) => {
                hostname: None,
                port: None,
                socketPath: Some(socketPath),
                path,
                method: req->IncommingMessage.method,
                headers: req->IncommingMessage.headers,
              }
            | Url(url) => {
                hostname: Some(url->Url.hostname),
                port:
                  Some(
                    url
                    ->Url.port
                    ->Belt.Int.fromString
                    ->Belt.Option.getWithDefault(80),
                  ),
                socketPath: None,
                path,
                method: req->IncommingMessage.method,
                headers: req->IncommingMessage.headers,
              }
            };
          };
        };
      };

      let proxyReq =
        nodeRequest(targetOptions, targetRes =>
          if (targetRes->IncommingMessage.statusCode == 404) {
            res
            ->ServerResponse.writeHead(
                ~statusCode=404,
                ~headers=
                  Some(
                    Js.Dict.fromArray([|("Content-Type", "text/html")|]),
                  ),
              )
            ->ServerResponse.end_("<h1>Page not found</h1>");
          } else {
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
          }
        );

      proxyReq->ClientRequest.on("error", error => {
        Js.Console.error2("Error with proxy request:", error);
        res
        ->ServerResponse.writeHead(~statusCode=404, ~headers=None)
        ->ServerResponse.end_("Internal server error");
      });

      req->IncommingMessage.pipeToClientRequest(proxyReq, {end_: true});
    });

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

      Js.Global.setTimeout(
        () => {
          Js.log("[Dev server] Failed to gracefully shutdown.");
          Process.exit(1);
        },
        GracefulShutdown.gracefulShutdownTimeout,
      )
      ->ignore;
    });
  });

  ();
};
