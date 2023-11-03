let makeMinimalPrintablePageObj = (~pagePath: PagePath.t, ~pageModulePath) => {
  let pagePath = PagePath.toString(pagePath);
  {"Page path": pagePath, "Page module path": pageModulePath};
};

type level =
  | Info
  | Debug;

let levelToIndex = (level: level) =>
  switch (level) {
  | Info => 0
  | Debug => 1
  };

let log = (~logLevel: level, ~messageLevel: level, log: unit => unit) => {
  let logLevelNum = levelToIndex(logLevel);
  let levelIndexNum = levelToIndex(messageLevel);
  if (logLevelNum >= levelIndexNum) {
    log();
  };
};

type logger = {
  logLevel: level,
  info: (unit => unit) => unit,
  debug: (unit => unit) => unit,
};

let makeLogger = (logLevel: level) => {
  {
    logLevel,
    info: log(~logLevel, ~messageLevel=Info),
    debug: log(~logLevel, ~messageLevel=Debug),
  };
};
