type t =
  | PageWithoutData
  | PageWithData
  | PageWithoutDataAndWrapperWithoutData
  | PageWithoutDataAndWrapperWithData
  | PageWithDataAndWrapperWithoutData
  | PageWithDataAndWrapperWithData

let toSlug = page =>
  switch page {
  | PageWithoutData => "page-without-data"
  | PageWithData => "page-with-data"
  | PageWithoutDataAndWrapperWithoutData => "page-without-data-and-wrapper-without-data"
  | PageWithoutDataAndWrapperWithData => "page-without-data-and-wrapper-with-data"
  | PageWithDataAndWrapperWithoutData => "page-with-data-and-wrapper-without-data"
  | PageWithDataAndWrapperWithData => "page-with-data-and-wrapper-with-data"
  }

let all = [
  PageWithoutData,
  PageWithData,
  PageWithoutDataAndWrapperWithoutData,
  PageWithoutDataAndWrapperWithData,
  PageWithDataAndWrapperWithoutData,
  PageWithDataAndWrapperWithData,
]
