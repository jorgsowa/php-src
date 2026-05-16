--TEST--
Test is_a() function : usage variations  - case sensitivity
--FILE--
<?php
echo "*** Testing is_a() : usage variations ***\n";

class caseSensitivityTest {}
class caseSensitivityTestChild extends caseSensitivityTest {}

var_dump(is_a(new caseSensitivityTestChild, 'caseSensitivityTEST'));

echo "Done";
?>
--EXPECTF--
*** Testing is_a() : usage variations ***

Deprecated: Using caseSensitivityTEST as a class name with incorrect case is deprecated, use the correct casing caseSensitivityTest instead in %s on line %d
bool(true)
Done
