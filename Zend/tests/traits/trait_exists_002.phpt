--TEST--
Testing trait_exists() inside a namespace
--FILE--
<?php

namespace foo;

trait IFoo { }

trait ITest { }


var_dump(trait_exists('IFoo'));
var_dump(trait_exists('foo\\IFoo'));
var_dump(trait_exists('FOO\\ITEST'));

?>
--EXPECTF--
bool(false)
bool(true)

Deprecated: Using FOO\ITEST as a class name with incorrect case is deprecated, use the correct casing foo\ITest instead in %s on line %d
bool(true)
