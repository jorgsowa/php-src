--TEST--
Testing interface_exists() inside a namespace
--FILE--
<?php

namespace foo;

interface IFoo { }

interface ITest extends IFoo { }

interface IBar extends IFoo { }


var_dump(interface_exists('IFoo'));
var_dump(interface_exists('foo\\IFoo'));
var_dump(interface_exists('FOO\\ITEST'));

?>
--EXPECTF--
bool(false)
bool(true)

Deprecated: Using FOO\ITEST as a class name with incorrect case is deprecated, use the correct casing foo\ITest instead in %s on line %d
bool(true)
