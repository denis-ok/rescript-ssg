[@mel.module "node:perf_hooks"] [@mel.scope "performance"]
external now: unit => float = "now";

let durationSinceStartTime = (~startTime) =>
  ((now() -. startTime) |> Js.Float.toFixed(~digits=2))++ " ms";
