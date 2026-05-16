--TEST--
ReflectionClassConstant with wrong-case class name emits E_DEPRECATED
--FILE--
<?php
class MyClass {
    const MY_CONST = "hello";
}

$rcc = new ReflectionClassConstant("myclass", "MY_CONST");
echo $rcc->getValue() . "\n";

$rcc2 = new ReflectionClassConstant("MYCLASS", "MY_CONST");
echo $rcc2->getValue() . "\n";
?>
--EXPECTF--
Deprecated: Using myclass as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
hello

Deprecated: Using MYCLASS as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
hello
