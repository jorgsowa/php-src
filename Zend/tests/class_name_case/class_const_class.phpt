--TEST--
Class name with incorrect case is deprecated in ::class constant
--FILE--
<?php
class Foo {}

// Correct case — no deprecation
var_dump(Foo::class);

// Wrong case — E_DEPRECATED at compile time
var_dump(FOO::class);
var_dump(foo::class);

// self::class and $obj::class — no deprecation, casing comes from the engine
class Bar {
    public static function test(): void {
        var_dump(self::class);
    }
}
Bar::test();

$obj = new Foo();
var_dump($obj::class);

// Class not known at compile time — ::class folds silently; the warning fires
// when the returned string is consumed by a real lookup, not here
var_dump(NotDeclaredYet::class);
?>
--EXPECTF--

Deprecated: Using FOO as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d

Deprecated: Using foo as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
string(3) "Foo"
string(3) "FOO"
string(3) "foo"
string(3) "Bar"
string(3) "Foo"
string(14) "NotDeclaredYet"
