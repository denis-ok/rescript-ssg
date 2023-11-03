[@bs.module "node:path"] external join2: (string, string) => string = "join";
[@bs.module "node:path"]
external join3: (string, string, string) => string = "join";
[@bs.module "node:path"] external basename: string => string = "basename";
[@bs.module "node:path"] external extname: string => string = "extname";
[@bs.module "node:path"] external dirname: string => string = "dirname";
[@bs.module "node:path"]
external relative: (~from: string, ~to_: string) => string = "relative";
