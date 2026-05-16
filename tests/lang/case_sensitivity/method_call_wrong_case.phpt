--TEST--
Instance method called with wrong case emits E_DEPRECATED
--FILE--
<?php
class Foo {
    public function myMethod() { return "ok"; }
}
$obj = new Foo();
echo $obj->MYMETHOD() . "\n";
echo $obj->myMethod() . "\n";
?>
--EXPECTF--
Deprecated: Calling MYMETHOD() is deprecated, use the correct casing Foo::myMethod() instead in %s on line %d
ok
ok
