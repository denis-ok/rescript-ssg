[@mel.module "node:path"] external join2: (string, string) => string = "join";
[@mel.module "node:path"]
external join3: (string, string, string) => string = "join";
[@mel.module "node:path"] external basename: string => string = "basename";
[@mel.module "node:path"] external extname: string => string = "extname";
[@mel.module "node:path"] external dirname: string => string = "dirname";
[@mel.module "node:path"]
external relative: (~from: string, ~to_: string) => string = "relative";
