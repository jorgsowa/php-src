--TEST--
Bug #81046: Literal compaction merges non-equal related literals
--FILE--
<?php

class Test {
	static function methoD() {
        echo "Method called\n";
	}
}

const methoD = 1;
var_dump(methoD);
test::methoD();

?>
--EXPECTF--
int(1)

Deprecated: Using test as a class name with incorrect case is deprecated, use the correct casing Test instead in %s on line %d
Method called
