[@mel.send]
external flat1: (array(array('a)), [@mel.as 1] _) => array('a) = "flat";

[@mel.send]
external flat2: (array(array(array('a))), [@mel.as 2] _) => array('a) =
  "flat";

let splitIntoChunks = (array: array('a), ~chunkSize): array(array('a)) => {
  let length = Js.Array.length(array);
  let chunksCount = length / chunkSize + (length mod chunkSize > 0 ? 1 : 0);
  let tempArray = Belt.Array.make(chunksCount, ());
  let (_, acc) =
    tempArray->Js.Array.reduce(
      ~f=((rest, acc), _tempChunk) => {
        let chunk = rest->Js.Array.slice(~start=0, ~end_=chunkSize, _);
        let rest = rest->Js.Array.slice(~start=chunkSize, _);
        let newAcc = Js.Array.concat(acc, [|chunk|]);
        (rest, newAcc);
      },
      ~init=(array, [||]),
      _
    );
  acc;
};
