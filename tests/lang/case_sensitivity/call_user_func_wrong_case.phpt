--TEST--
call_user_func with wrong case emits E_DEPRECATED
--FILE--
<?php
$result = call_user_func("STRLEN", "hello");
echo $result . "\n";

$result2 = call_user_func("strlen", "hello");
echo $result2 . "\n";
?>
--EXPECTF--
Deprecated: Calling STRLEN() is deprecated, use the correct casing strlen() instead in %s on line %d
5
5
