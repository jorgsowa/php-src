--TEST--
ReflectionClass::isSubclassOf() with wrong-case class name emits E_DEPRECATED
--FILE--
<?php
class Base {}
class Child extends Base {}

$rc = new ReflectionClass(Child::class);

var_dump($rc->isSubclassOf("base"));
var_dump($rc->isSubclassOf("BASE"));
var_dump($rc->isSubclassOf("Base"));
?>
--EXPECTF--
Deprecated: Using base as a class name with incorrect case is deprecated, use the correct casing Base instead in %s on line %d
bool(true)

Deprecated: Using BASE as a class name with incorrect case is deprecated, use the correct casing Base instead in %s on line %d
bool(true)
bool(true)
