[@send] external flat': (array(array('a)), int) => array('a) = "flat";

let flat1 = array => array->flat'(1);

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
