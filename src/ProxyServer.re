module Server = {
  type t;
  [@send] external listen: (t, int, unit => unit) => unit = "listen";
};

// https://nodejs.org/api/http.html#class-httpclientrequest
type clientRequest;

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
  external writeHead: (t, int, Js.Dict.t(string)) => unit = "writeHead";

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
  external pipeToClientRequest: (t, clientRequest, pipeOptions) => unit =
    "pipe";
};

[@bs.module "node:http"]
external nodeCreateServer:
  ((IncommingMessage.t, ServerResponse.t) => unit) => Server.t =
  "createServer";

[@bs.module "node:http"]
external nodeRequest:
  (nodeRequestOptions, IncommingMessage.t => unit) => clientRequest =
  "request";

let start = (~port: int, ~targetHost: string, ~targetPort: int) => {
  let server =
    nodeCreateServer((req, res) => {
      let options = {
        hostname: Some(targetHost),
        port: Some(targetPort),
        path: req->IncommingMessage.url,
        method: req->IncommingMessage.method,
        headers: req->IncommingMessage.headers,
        socketPath: None,
      };

      let proxy =
        nodeRequest(options, targetRes =>
          if (targetRes->IncommingMessage.statusCode == 404) {
            res->ServerResponse.writeHead(
              404,
              Js.Dict.fromArray([|("Content-Type", "text/html")|]),
            );
            res->ServerResponse.end_("<h1>Page not found</h1>");
          } else {
            res->ServerResponse.writeHead(
              targetRes->IncommingMessage.statusCode,
              targetRes->IncommingMessage.headers,
            );
            targetRes->IncommingMessage.pipeToServerResponse(
              res,
              {end_: true},
            );
          }
        );

      req->IncommingMessage.pipeToClientRequest(proxy, {end_: true});
    });

  server->Server.listen(port, () =>
    Js.log("[Dev server] Listening on port " ++ string_of_int(port))
  );

  ();
};
