--TEST--
Class name with incorrect case is deprecated in class constant access
--FILE--
<?php
class Foo {
    const BAR = 42;
}

var_dump(Foo::BAR);   // correct case, no deprecation
var_dump(FOO::BAR);   // wrong case
var_dump(foo::BAR);   // wrong case
?>
--EXPECTF--

Deprecated: Using FOO as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d

Deprecated: Using foo as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
int(42)
int(42)
int(42)
