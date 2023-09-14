type t('a);
[@mel.new] external fromArray: array('a) => t('a) = "Set";

type arrayModule;

[@mel.val] external arrayModule: arrayModule = "Array";
[@mel.send] external arrayFrom: (arrayModule, t('a)) => array('a) = "from";

let toArray = set => arrayModule->arrayFrom(set);
