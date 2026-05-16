--TEST--
Test get_parent_class() function : variation - case sensitivity
--FILE--
<?php
//  Note: basic use cases in Zend/tests/010.phpt

echo "*** Testing get_parent_class() : variation ***\n";

class caseSensitivityTest {}
class caseSensitivityTestChild extends caseSensitivityTest {}

var_dump(get_parent_class('CasesensitivitytestCHILD'));
var_dump(get_parent_class(new CasesensitivitytestCHILD));

echo "Done";
?>
--EXPECTF--
*** Testing get_parent_class() : variation ***
string(19) "caseSensitivityTest"

Deprecated: Using CasesensitivitytestCHILD as a class name with incorrect case is deprecated, use the correct casing caseSensitivityTestChild instead in %s on line %d
string(19) "caseSensitivityTest"
Done
