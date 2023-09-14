let assetPrefix =
  Process.env
  ->Js.Dict.get("RESCRIPT_SSG_ASSET_PREFIX")
  ->Belt.Option.getWithDefault("");
