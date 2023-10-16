type t('a);
[@bs.new] external fromArray: array('a) => t('a) = "Set";

[@send] external has: (t('a), 'a) => bool = "has";

[@send] external add: (t('a), 'a) => t('a) = "add";

type arrayModule;

[@bs.val] external arrayModule: arrayModule = "Array";
[@bs.send] external arrayFrom: (arrayModule, t('a)) => array('a) = "from";

let toArray = set => arrayModule->arrayFrom(set);
