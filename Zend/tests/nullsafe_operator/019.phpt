--TEST--
Test nullsafe in new
--FILE--
<?php

class Bar {}

class Foo {
    public $bar;
}

$foo = new Foo();
$foo->bar = 'bar';
var_dump(new $foo?->bar);

$foo = null;
var_dump(new $foo?->bar);

?>
--EXPECTF--

Deprecated: Using bar as a class name with incorrect case is deprecated, use the correct casing Bar instead in %s on line %d
object(Bar)#2 (0) {
}

Fatal error: Uncaught Error: Class name must be a valid object or a string in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d
