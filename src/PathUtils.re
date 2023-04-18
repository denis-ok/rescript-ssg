let maybeAddSlashPrefix = path =>
  if (path->Js.String2.startsWith("http") || path->Js.String2.startsWith("/")) {
    path;
  } else {
    "/" ++ path;
  };

let maybeAddSlashSuffix = path =>
  if (path->Js.String2.endsWith("/")) {
    path;
  } else {
    path ++ "/";
  };
