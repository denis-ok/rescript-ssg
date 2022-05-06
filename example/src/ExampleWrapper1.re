let wrapper = (children, str) => {
  <span>
    {React.string("Hello from wrapper component for all pages: " ++ str)}
    children
  </span>;
};

let wrapperReference = __MODULE__ ++ ".wrapper";
