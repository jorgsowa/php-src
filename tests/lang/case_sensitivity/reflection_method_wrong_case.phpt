--TEST--
ReflectionMethod with wrong-case class name emits E_DEPRECATED
--FILE--
<?php
class MyClass {
    public function myMethod(): void {}
}

$rm = new ReflectionMethod("myclass", "myMethod");
echo $rm->getName() . "\n";

$rm2 = new ReflectionMethod("MYCLASS", "myMethod");
echo $rm2->getName() . "\n";
?>
--EXPECTF--
Deprecated: Using myclass as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
myMethod

Deprecated: Using MYCLASS as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
myMethod
