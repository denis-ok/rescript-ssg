[@mel.send]
external flat1: (array(array('a)), [@mel.as 1] _) => array('a) = "flat";

[@mel.send]
external flat2: (array(array(array('a))), [@mel.as 2] _) => array('a) =
  "flat";

let splitIntoChunks = (array: array('a), ~chunkSize): array(array('a)) => {
  let length = Js.Array2.length(array);
  let chunksCount = length / chunkSize + (length mod chunkSize > 0 ? 1 : 0);
  let tempArray = Belt.Array.make(chunksCount, ());
  let (_, acc) =
    tempArray->Js.Array2.reduce(
      ((rest, acc), _tempChunk) => {
        let chunk = rest->Js.Array2.slice(~start=0, ~end_=chunkSize);
        let rest = rest->Js.Array2.sliceFrom(chunkSize);
        let newAcc = Js.Array2.concat(acc, [|chunk|]);
        (rest, newAcc);
      },
      (array, [||]),
    );
  acc;
};
