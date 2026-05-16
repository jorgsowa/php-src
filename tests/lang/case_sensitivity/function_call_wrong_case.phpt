--TEST--
Function called with wrong case emits E_DEPRECATED
--FILE--
<?php
$result = STRLEN("hello");
echo $result . "\n";
?>
--EXPECTF--
Deprecated: Calling STRLEN() is deprecated, use the correct casing strlen() instead in %s on line %d
5
