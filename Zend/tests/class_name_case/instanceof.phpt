--TEST--
Class name with incorrect case is deprecated in instanceof operator
--FILE--
<?php
class Foo {}

$o = new Foo();

var_dump($o instanceof Foo);   // correct case, no deprecation
var_dump($o instanceof FOO);   // wrong case
var_dump($o instanceof foo);   // wrong case
?>
--EXPECTF--
bool(true)

Deprecated: Using FOO as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
bool(true)

Deprecated: Using foo as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
bool(true)
