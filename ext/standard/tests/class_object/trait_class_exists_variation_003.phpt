--TEST--
Test trait_exists() function : usage variations  - case sensitivity
--FILE--
<?php
trait caseSensitivityTest {}
var_dump(trait_exists('casesensitivitytest'));

echo "Done"
?>
--EXPECTF--
Deprecated: Using casesensitivitytest as a class name with incorrect case is deprecated, use the correct casing caseSensitivityTest instead in %s on line %d
bool(true)
Done
