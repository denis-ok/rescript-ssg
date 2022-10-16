let makeMinimalPrintablePageObj =
    (~pagePath: PageBuilderT.PagePath.t, ~pageModulePath) => {
  let pagePath = PageBuilderT.PagePath.toString(pagePath);
  {"Page path": pagePath, "Page module path": pageModulePath};
};
