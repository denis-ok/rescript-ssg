[@bs.module "node:perf_hooks"] [@bs.scope "performance"]
external now: unit => float = "now";

let durationSinceStartTime = (~startTime) =>
  (now() -. startTime)->Js.Float.toFixedWithPrecision(~digits=2) ++ " ms";
