module PagePath = {
  type t =
    | Root
    | Path(array(string));

  let toString = t =>
    switch (t) {
    | Root => "."
    | Path(parts) => parts->Js.Array2.joinWith("/")
    };
};
