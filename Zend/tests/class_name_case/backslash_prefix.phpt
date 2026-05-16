--TEST--
Class name with leading backslash and incorrect case is deprecated
--FILE--
<?php
class Foo {}

$o = new Foo();

// Correct case with leading backslash - no deprecation
var_dump($o instanceof \Foo);
var_dump(is_a($o, '\Foo'));
var_dump(class_exists('\Foo'));

// Wrong case with leading backslash - should warn
var_dump(is_a($o, '\FOO'));
var_dump(class_exists('\FOO'));

// Dynamic new with leading backslash and wrong case
$cls = '\FOO';
$obj = new $cls();
var_dump($obj instanceof Foo);
?>
--EXPECTF--
bool(true)
bool(true)
bool(true)

Deprecated: Using FOO as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
bool(true)

Deprecated: Using FOO as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
bool(true)

Deprecated: Using FOO as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
bool(true)
