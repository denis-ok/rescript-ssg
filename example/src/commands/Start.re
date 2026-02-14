open RescriptSsg;

let currentDir = Utils.getDirname();

let () =
  Commands.start(
    ~melangeArtifactsExtension=Pages.melangeArtifactsExtension,
    ~pageAppArtifactsType=Js,
    ~pages=Pages.pages,
    ~globalEnvValues=Pages.globalEnvValues,
    ~outputDir=Pages.outputDir,
    ~melangeOutputDir=Pages.melangeOutputDir,
    ~projectRootDir=Pages.projectRootDir,
    ~logLevel=Info,
    ~buildWorkersCount=1,
    ~pageAppArtifactsSuffix=UnixTimestamp,
    (),
  );
