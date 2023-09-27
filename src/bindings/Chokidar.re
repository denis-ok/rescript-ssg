type chokidar;

type watcher;

[@mel.module "chokidar"] external chokidar: chokidar = "default";

[@mel.send] external watchFile: (chokidar, string) => watcher = "watch";

[@mel.send]
external watchFiles: (chokidar, array(string)) => watcher = "watch";

[@mel.send] external onEvent: (watcher, string, string => unit) => unit = "on";

[@mel.send]
external onEventWithUnitCallback: (watcher, string, unit => unit) => unit =
  "on";

[@mel.send] external add: (watcher, array(string)) => unit = "add";

[@mel.send] external unwatch: (watcher, array(string)) => unit = "unwatch";

[@mel.send]
external getWatched: (watcher, unit) => array(string) = "getWatched";

let onChange = (chokidar, callback) => chokidar->onEvent("change", callback);

let onUnlink = (chokidar, callback) => chokidar->onEvent("unlink", callback);

let onReady = (chokidar, callback) =>
  chokidar->onEventWithUnitCallback("ready", callback);
