[@mel.module "path"] external join2: (string, string) => string = "join";
[@mel.module "path"]
external join3: (string, string, string) => string = "join";
[@mel.module "path"] external basename: string => string = "basename";
[@mel.module "path"] external extname: string => string = "extname";
[@mel.module "path"] external dirname: string => string = "dirname";
[@mel.module "path"]
external relative: (~from: string, ~to_: string) => string = "relative";
