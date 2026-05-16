--TEST--
Test get_class() function : usage variations  - ensure class name case is preserved.
--FILE--
<?php
class caseSensitivityTest {}
var_dump(get_class(new casesensitivitytest));

echo "Done";
?>
--EXPECTF--
Deprecated: Using casesensitivitytest as a class name with incorrect case is deprecated, use the correct casing caseSensitivityTest instead in %s on line %d
string(19) "caseSensitivityTest"
Done
