--TEST--
Class name with incorrect case is deprecated in new operator
--FILE--
<?php
class Foo {}

$o = new Foo(); // correct case, no deprecation
var_dump($o instanceof Foo);

$o = new FOO(); // wrong case
var_dump($o instanceof Foo);

$o = new foo(); // wrong case
var_dump($o instanceof Foo);
?>
--EXPECTF--
bool(true)

Deprecated: Using FOO as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
bool(true)

Deprecated: Using foo as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
bool(true)
