type chokidar;

type watcher;

type event;

[@module "chokidar"] external chokidar: chokidar = "default";

[@send] external watchFile: (chokidar, string) => watcher = "watch";

[@send] external watchFiles: (chokidar, array(string)) => watcher = "watch";

[@send] external onEvent: (watcher, string, string => unit) => unit = "on";

[@send]
external onEventWithUnitCallback: (watcher, string, unit => unit) => unit =
  "on";

[@send] external add: (watcher, array(string)) => unit = "add";

[@send] external unwatch: (watcher, array(string)) => unit = "unwatch";

[@send] external getWatched: (watcher, unit) => array(string) = "getWatched";

let onChange = (chokidar, callback) => chokidar->onEvent("change", callback);

let onReady = (chokidar, callback) =>
  chokidar->onEventWithUnitCallback("ready", callback);
