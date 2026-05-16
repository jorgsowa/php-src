--TEST--
Class name with incorrect case is deprecated in dynamic new operator
--FILE--
<?php
class Foo {}

$c = 'Foo';
$o = new $c(); // correct case, no deprecation
var_dump($o instanceof Foo);

$c = 'FOO';
$o = new $c(); // wrong case
var_dump($o instanceof Foo);
?>
--EXPECTF--
bool(true)

Deprecated: Using FOO as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
bool(true)
