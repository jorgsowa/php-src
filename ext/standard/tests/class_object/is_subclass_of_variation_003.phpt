--TEST--
Test is_subclass_of() function : usage variations  - case sensitivity
--FILE--
<?php
echo "*** Testing is_subclass_of() : usage variations ***\n";

echo "*** Testing is_a() : usage variations ***\n";

class caseSensitivityTest {}
class caseSensitivityTestChild extends caseSensitivityTest {}

var_dump(is_subclass_of('caseSensitivityTestCHILD', 'caseSensitivityTEST'));

echo "Done"
?>
--EXPECTF--
*** Testing is_subclass_of() : usage variations ***
*** Testing is_a() : usage variations ***

Deprecated: Using caseSensitivityTestCHILD as a class name with incorrect case is deprecated, use the correct casing caseSensitivityTestChild instead in %s on line %d

Deprecated: Using caseSensitivityTEST as a class name with incorrect case is deprecated, use the correct casing caseSensitivityTest instead in %s on line %d
bool(true)
Done
