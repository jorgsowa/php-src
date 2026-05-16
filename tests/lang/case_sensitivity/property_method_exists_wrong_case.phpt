--TEST--
property_exists() and method_exists() with wrong-case class name emits E_DEPRECATED
--FILE--
<?php
class MyClass {
    public int $value = 1;
    public function myMethod(): void {}
}

var_dump(property_exists("MYCLASS", "value"));
var_dump(property_exists("myclass", "value"));
var_dump(method_exists("MYCLASS", "myMethod"));
var_dump(method_exists("myclass", "myMethod"));
?>
--EXPECTF--
Deprecated: Using MYCLASS as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
bool(true)

Deprecated: Using myclass as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
bool(true)

Deprecated: Using MYCLASS as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
bool(true)

Deprecated: Using myclass as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
bool(true)
