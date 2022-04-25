type chokidar

type watcher

type event

@module("chokidar") external chokidar: chokidar = "default"

@send external watchFile: (chokidar, string) => watcher = "watch"

@send external watchFiles: (chokidar, array<string>) => watcher = "watch"

@send external on: (watcher, string, (event, string) => unit) => unit = "on"
