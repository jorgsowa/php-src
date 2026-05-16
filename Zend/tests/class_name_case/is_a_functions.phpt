--TEST--
Class name with incorrect case is deprecated in is_a and is_subclass_of
--FILE--
<?php
class Foo extends Exception {}

$o = new Foo();

// correct case, no deprecation
var_dump(is_a($o, 'Foo'));
var_dump(is_subclass_of($o, 'Exception'));

// wrong case
var_dump(is_a($o, 'FOO'));
var_dump(is_subclass_of($o, 'EXCEPTION'));
?>
--EXPECTF--
bool(true)
bool(true)

Deprecated: Using FOO as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
bool(true)

Deprecated: Using EXCEPTION as a class name with incorrect case is deprecated, use the correct casing Exception instead in %s on line %d
bool(true)
