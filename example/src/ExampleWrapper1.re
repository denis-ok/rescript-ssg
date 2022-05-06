let wrapper = (children, str) => {
  <span> {React.string("Wrapper prop: " ++ str)} children </span>;
};

let wrapperReference = __MODULE__ ++ ".wrapper";
