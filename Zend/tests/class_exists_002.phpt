--TEST--
Testing several valid and invalid parameters
--FILE--
<?php

class foo {

}

var_dump(class_exists(''));
var_dump(class_exists('FOO'));
var_dump(class_exists('bar'));
var_dump(class_exists(1));

?>
--EXPECTF--
bool(false)

Deprecated: Using FOO as a class name with incorrect case is deprecated, use the correct casing foo instead in %s on line %d
bool(true)
bool(false)
bool(false)
