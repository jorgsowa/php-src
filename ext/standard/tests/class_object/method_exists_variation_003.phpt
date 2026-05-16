--TEST--
Test method_exists() function : variation - Case sensitivity
--FILE--
<?php
echo "*** Testing method_exists() : variation ***\n";

Class caseSensitivityTest {
    public function myMethod() {}
}

var_dump(method_exists(new casesensitivitytest, 'myMetHOD'));
var_dump(method_exists('casesensiTivitytest', 'myMetHOD'));

echo "Done";
?>
--EXPECTF--
*** Testing method_exists() : variation ***

Deprecated: Using casesensitivitytest as a class name with incorrect case is deprecated, use the correct casing caseSensitivityTest instead in %s on line %d

Deprecated: Calling myMetHOD() is deprecated, use the correct casing caseSensitivityTest::myMethod() instead in %s on line %d
bool(true)

Deprecated: Using casesensiTivitytest as a class name with incorrect case is deprecated, use the correct casing caseSensitivityTest instead in %s on line %d

Deprecated: Calling myMetHOD() is deprecated, use the correct casing caseSensitivityTest::myMethod() instead in %s on line %d
bool(true)
Done
