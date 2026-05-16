--TEST--
User-defined function called with wrong case emits E_DEPRECATED
--FILE--
<?php
function myUserFunction() {
    return "user func result";
}
echo MYUSERFUNCTION() . "\n";
echo myUserFunction() . "\n";
?>
--EXPECTF--
Deprecated: Calling MYUSERFUNCTION() is deprecated, use the correct casing myUserFunction() instead in %s on line %d
user func result
user func result
