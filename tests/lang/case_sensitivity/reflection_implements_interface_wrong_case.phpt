--TEST--
ReflectionClass::implementsInterface() with wrong-case interface name emits E_DEPRECATED
--FILE--
<?php
interface MyInterface {}
class MyClass implements MyInterface {}

$rc = new ReflectionClass(MyClass::class);

var_dump($rc->implementsInterface("myinterface"));
var_dump($rc->implementsInterface("MYINTERFACE"));
var_dump($rc->implementsInterface("MyInterface"));
?>
--EXPECTF--
Deprecated: Using myinterface as a class name with incorrect case is deprecated, use the correct casing MyInterface instead in %s on line %d
bool(true)

Deprecated: Using MYINTERFACE as a class name with incorrect case is deprecated, use the correct casing MyInterface instead in %s on line %d
bool(true)
bool(true)
