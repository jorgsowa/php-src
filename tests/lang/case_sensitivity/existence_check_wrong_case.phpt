--TEST--
function_exists and method_exists with wrong case emit E_DEPRECATED
--FILE--
<?php
class MyClass {
    public function myMethod() {}
}

var_dump(function_exists("STRLEN"));
var_dump(function_exists("strlen"));
var_dump(method_exists("MyClass", "MYMETHOD"));
var_dump(method_exists("MyClass", "myMethod"));
?>
--EXPECTF--
Deprecated: Calling STRLEN() is deprecated, use the correct casing strlen() instead in %s on line %d
bool(true)
bool(true)

Deprecated: Calling MYMETHOD() is deprecated, use the correct casing MyClass::myMethod() instead in %s on line %d
bool(true)
bool(true)
