type chokidar;

type watcher;

[@bs.module "chokidar"] external chokidar: chokidar = "default";

[@bs.send] external watchFile: (chokidar, string) => watcher = "watch";

[@bs.send]
external watchFiles: (chokidar, array(string)) => watcher = "watch";

[@bs.send] external onEvent: (watcher, string, string => unit) => unit = "on";

[@bs.send]
external onEventWithUnitCallback: (watcher, string, unit => unit) => unit =
  "on";

[@bs.send] external add: (watcher, array(string)) => unit = "add";

[@bs.send] external unwatch: (watcher, array(string)) => unit = "unwatch";

[@bs.send]
external getWatched: (watcher, unit) => array(string) = "getWatched";

let onChange = (chokidar, callback) => chokidar->onEvent("change", callback);

let onUnlink = (chokidar, callback) => chokidar->onEvent("unlink", callback);

let onReady = (chokidar, callback) =>
  chokidar->onEventWithUnitCallback("ready", callback);

let onAdd = (chokidar, callback) =>
  chokidar->onEventWithUnitCallback("add", callback);
