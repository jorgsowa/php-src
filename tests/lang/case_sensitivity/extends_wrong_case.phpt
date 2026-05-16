--TEST--
Extending a class with wrong-case parent name emits E_DEPRECATED
--FILE--
<?php
require __DIR__ . "/extends_wrong_case_base.inc";

class Child extends BASECLASS {}
echo "done\n";
?>
--EXPECTF--
Deprecated: Using BASECLASS as a class name with incorrect case is deprecated, use the correct casing BaseClass instead in %s on line %d
done
