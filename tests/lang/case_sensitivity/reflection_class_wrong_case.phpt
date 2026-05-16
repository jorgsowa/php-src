--TEST--
ReflectionClass with wrong-case class name emits E_DEPRECATED
--FILE--
<?php
class MyClass {
    public int $value = 42;
}

$rc = new ReflectionClass("myclass");
echo $rc->getName() . "\n";

$rc2 = new ReflectionClass("MYCLASS");
echo $rc2->getName() . "\n";
?>
--EXPECTF--
Deprecated: Using myclass as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
MyClass

Deprecated: Using MYCLASS as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
MyClass
