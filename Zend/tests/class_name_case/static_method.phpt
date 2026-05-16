--TEST--
Class name with incorrect case is deprecated in static method call
--FILE--
<?php
class Foo {
    static function bar(): string { return 'ok'; }
}

echo Foo::bar() . "\n"; // correct case, no deprecation
echo FOO::bar() . "\n"; // wrong case
echo foo::bar() . "\n"; // wrong case
?>
--EXPECTF--
ok

Deprecated: Using FOO as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
ok

Deprecated: Using foo as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
ok
