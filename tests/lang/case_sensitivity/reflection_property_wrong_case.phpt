--TEST--
ReflectionProperty with wrong-case class name emits E_DEPRECATED
--FILE--
<?php
class MyClass {
    public int $value = 42;
}

$rp = new ReflectionProperty("myclass", "value");
echo $rp->getName() . "\n";

$rp2 = new ReflectionProperty("MYCLASS", "value");
echo $rp2->getName() . "\n";
?>
--EXPECTF--
Deprecated: Using myclass as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
value

Deprecated: Using MYCLASS as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
value
