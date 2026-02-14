open RescriptSsg;

let build = () =>
  Commands.build(
    ~melangeArtifactsExtension=Pages.melangeArtifactsExtension,
    ~pageAppArtifactsType=Js,
    ~pages=Pages.pages,
    ~globalEnvValues=Pages.globalEnvValues,
    ~outputDir=Pages.outputDir,
    ~melangeOutputDir=Pages.melangeOutputDir,
    ~projectRootDir=Pages.projectRootDir,
    ~logLevel=Info,
    // compileCommand isn't used with pageAppArtifactsType=Js
    // Should be removed/refactored in the future
    ~compileCommand="dune build",
    ~buildWorkersCount=1,
    ~pageAppArtifactsSuffix=NoSuffix,
    (),
  )
  ->Promise.map(_ => Js.log("[rescript-ssg] Build success!"))
  ->ignore;
