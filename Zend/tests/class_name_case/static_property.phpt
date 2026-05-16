--TEST--
Class name with incorrect case is deprecated in static property access
--FILE--
<?php
class Foo {
    static int $x = 42;
}

var_dump(Foo::$x);   // correct case, no deprecation
var_dump(FOO::$x);   // wrong case
var_dump(foo::$x);   // wrong case
?>
--EXPECTF--
int(42)

Deprecated: Using FOO as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
int(42)

Deprecated: Using foo as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
int(42)
