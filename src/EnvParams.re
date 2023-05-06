let assetPrefix =
  Process.env->Js.Dict.get("ASSET_PREFIX")->Belt.Option.getWithDefault("");
