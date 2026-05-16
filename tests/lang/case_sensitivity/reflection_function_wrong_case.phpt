--TEST--
ReflectionFunction with wrong-case function name emits E_DEPRECATED
--FILE--
<?php
function myFunc(): int {
    return 42;
}

$rf = new ReflectionFunction("MYFUNC");
echo $rf->getName() . "\n";

$rf2 = new ReflectionFunction("myfunc");
echo $rf2->getName() . "\n";
?>
--EXPECTF--
Deprecated: Calling MYFUNC() is deprecated, use the correct casing myFunc() instead in %s on line %d
myFunc

Deprecated: Calling myfunc() is deprecated, use the correct casing myFunc() instead in %s on line %d
myFunc
