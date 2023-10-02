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

module URL = {
  type t;
  [@bs.new] [@bs.scope "global"]
  external makeExn: (string, string) => t = "URL";
  [@bs.get] external hash: t => string = "hash";
  [@bs.get] external host: t => string = "host";
  [@bs.get] external hostname: t => string = "hostname";
  [@bs.get] external href: t => string = "href";
  [@bs.get] external origin: t => string = "origin";
  [@bs.get] external protocol: t => string = "protocol";
  [@bs.get] external pathname: t => string = "pathname";
  [@bs.get] external port: t => string = "port";
  [@bs.get] external search: t => string = "search";
  [@bs.get] external searchParams: t => Js.Dict.t(string) = "searchParams";

  let make = (path, base) =>
    switch (makeExn(path, base)) {
    | url => Some(url)
    | exception _ => None
    };
};

module ProxyRule = {
  type target = {
    host: option(string),
    port: option(int),
    unixSocket: option(string),
  };

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

let start =
    (
      ~port: int,
      ~targetHost: string,
      ~targetPort: int,
      ~proxyRules: array(ProxyRule.t),
    ) => {
  let server =
    nodeCreateServer((req, res) => {
      let reqUrl = req->IncommingMessage.url;
      Js.log2("Request to:", reqUrl);
      let reqHeaders = req->IncommingMessage.headers;
      let reqHost =
        reqHeaders->Js.Dict.get("host")->Belt.Option.getWithDefault("");
      let urlBase = "http://" ++ reqHost;
      let url = URL.make(reqUrl, urlBase);
      let path =
        switch (url) {
        | None => reqUrl
        | Some(url) => url->URL.pathname
        };

      let matchedRule =
        proxyRules->Js.Array2.find(rule =>
          path->Js.String2.startsWith(rule.from)
        );

      let targetOptions: nodeRequestOptions =
        switch (matchedRule) {
        | Some({from: _, to_: {target, pathRewrite}} as proxyRule) =>
          Js.log2("Proxy rule matched:", proxyRule);
          let path =
            switch (pathRewrite) {
            | Some({rewriteFrom, rewriteTo}) =>
              let newPath = path->Js.String2.replace(rewriteFrom, rewriteTo);
              Js.log2("Path rewritten, new path:", newPath);
              newPath;
            | None => path
            };

          {
            hostname: target.host,
            port: target.port,
            socketPath: target.unixSocket,
            path,
            method: req->IncommingMessage.method,
            headers: req->IncommingMessage.headers,
          };
        | None => {
            hostname: Some(targetHost),
            port: Some(targetPort),
            path: req->IncommingMessage.url,
            method: req->IncommingMessage.method,
            headers: req->IncommingMessage.headers,
            socketPath: None,
          }
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
