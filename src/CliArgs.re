let getProcessArgValue = (name: string) => {
  let prefix = {j|--$(name)=|j};

  let arg =
    Process.getArgs()
    ->Js.Array2.find(arg => arg->Js.String2.startsWith(prefix));

  let value =
    switch (arg) {
    | None => ""
    | Some(arg) => arg->Js.String2.replace(prefix, "")->Js.String2.trim
    };

  value;
};

let assetPrefix = getProcessArgValue("asset-prefix");
