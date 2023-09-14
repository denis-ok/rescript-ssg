[@bs.module "path"] external join2: (string, string) => string = "join";
[@bs.module "path"] external join3: (string, string, string) => string = "join";
[@bs.module "path"] external basename: string => string = "basename";
[@bs.module "path"] external extname: string => string = "extname";
[@bs.module "path"] external dirname: string => string = "dirname";
[@bs.module "path"]
external relative: (~from: string, ~to_: string) => string = "relative";
