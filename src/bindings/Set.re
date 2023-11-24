type t('a);
[@mel.new] external fromArray: array('a) => t('a) = "Set";

[@send] external has: (t('a), 'a) => bool = "has";

[@send] external add: (t('a), 'a) => t('a) = "add";

type arrayModule;

external arrayModule: arrayModule = "Array";
[@send] external arrayFrom: (arrayModule, t('a)) => array('a) = "from";

let toArray = set => arrayModule->arrayFrom(set);
