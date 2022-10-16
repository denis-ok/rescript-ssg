let makeMinimalPrintablePageObj =
    (~pagePath: PageBuilderT.PagePath.t, ~pageModulePath) => {
  let pagePath = PageBuilderT.PagePath.toString(pagePath);
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

let log = (~logSetting: level, ~level: level, log: unit => unit) => {
  let logSettingNum = levelToIndex(logSetting);
  let levelIndexNum = levelToIndex(level);
  if (logSettingNum >= levelIndexNum) {
    log();
  };
};

type logger = {
  logSetting: level,
  info: (unit => unit) => unit,
  debug: (unit => unit) => unit,
};

let makeLogger = (logSetting: level) => {
  {
    logSetting,
    info: log(~logSetting, ~level=Info),
    debug: log(~logSetting, ~level=Debug),
  };
};
