type t('a);
[@mel.new] external fromArray: array('a) => t('a) = "Set";

type arrayModule;

external arrayModule: arrayModule = "Array";
[@send] external arrayFrom: (arrayModule, t('a)) => array('a) = "from";

let toArray = set => arrayModule->arrayFrom(set);
