--TEST--
Class name with incorrect case is deprecated in class_exists, interface_exists, trait_exists
--FILE--
<?php
class Foo {}
interface IFoo {}
trait TFoo {}

// correct case, no deprecation
var_dump(class_exists('Foo'));
var_dump(interface_exists('IFoo'));
var_dump(trait_exists('TFoo'));

// wrong case
var_dump(class_exists('FOO'));
var_dump(interface_exists('IFOO'));
var_dump(trait_exists('TFOO'));
?>
--EXPECTF--
bool(true)
bool(true)
bool(true)

Deprecated: Using FOO as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
bool(true)

Deprecated: Using IFOO as a class name with incorrect case is deprecated, use the correct casing IFoo instead in %s on line %d
bool(true)

Deprecated: Using TFOO as a class name with incorrect case is deprecated, use the correct casing TFoo instead in %s on line %d
bool(true)
