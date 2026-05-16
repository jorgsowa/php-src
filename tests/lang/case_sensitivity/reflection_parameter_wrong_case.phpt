--TEST--
ReflectionParameter with wrong-case class name in array callable emits E_DEPRECATED
--FILE--
<?php
class MyClass {
    public function myMethod(int $x, string $y): void {}
}

$rp = new ReflectionParameter(["MYCLASS", "myMethod"], 0);
echo $rp->getName() . "\n";

$rp2 = new ReflectionParameter(["myclass", "myMethod"], 1);
echo $rp2->getName() . "\n";
?>
--EXPECTF--
Deprecated: Using MYCLASS as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
x

Deprecated: Using myclass as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
y
