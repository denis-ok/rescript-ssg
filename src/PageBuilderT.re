module PagePath = {
  type t =
    | Root
    | Path(array(string));

  let toString = t =>
    switch (t) {
    | Root => "."
    | Path(parts) => parts->Js.Array2.joinWith("/")
    };

  let toWebpackEntryName = t =>
    switch (t) {
    | Root => "root"
    | Path(parts) => parts->Js.Array2.joinWith("/")
    };
};
