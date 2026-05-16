--TEST--
Static property access with wrong-cased class name emits E_DEPRECATED
--FILE--
<?php
class Counter {
    public static int $value = 0;
}

// Wrong-cased class name on a static property write
COUNTER::$value = 5;

// Correct casing - no warning
echo Counter::$value, "\n";

// Wrong-cased class name on a static property read
echo COUNTER::$value, "\n";
?>
--EXPECTF--
Deprecated: Using COUNTER as a class name with incorrect case is deprecated, use the correct casing Counter instead in %s on line 7
5

Deprecated: Using COUNTER as a class name with incorrect case is deprecated, use the correct casing Counter instead in %s on line 13
5
