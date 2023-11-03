let assetPrefix =
  switch (Process.env->Js.Dict.get("RESCRIPT_SSG_ASSET_PREFIX")) {
  | None => ""
  | Some(assetPrefix) => assetPrefix
  };
