--TEST--
Dynamic function call with wrong case emits E_DEPRECATED
--FILE--
<?php
$fn = "STRLEN";
echo $fn("hello") . "\n";

$fn2 = "strlen";
echo $fn2("hello") . "\n";
?>
--EXPECTF--
Deprecated: Calling STRLEN() is deprecated, use the correct casing strlen() instead in %s on line %d
5
5
