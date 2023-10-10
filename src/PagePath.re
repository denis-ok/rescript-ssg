type t =
  | Root
  | Path(array(string));

let toString = t =>
  switch (t) {
  | Root => "."
  | Path(segments) => segments->Js.Array2.joinWith("/")
  };

let toWebpackEntryName = t =>
  switch (t) {
  | Root => "root"
  | Path(segments) => segments->Js.Array2.joinWith("/")
  };

let dynamicSegment = "__dynamic";
