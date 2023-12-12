type t =
  | Root
  | Path(array(string));

let toString = t =>
  switch (t) {
  | Root => "."
  | Path(segments) => segments->Js.Array.join(~sep="/", _)
  };

let toWebpackEntryName = t =>
  switch (t) {
  | Root => "root"
  | Path(segments) => segments->Js.Array.join(~sep="/", _)
  };

let dynamicSegment = "dynamic_segment";
