--TEST--
Property type declarations with wrong-cased class names emit E_DEPRECATED
--FILE--
<?php
class MyClass {}

class Container {
    public MYCLASS $value;
}

$c = new Container();
$c->value = new MyClass();
echo get_class($c->value) . "\n";
?>
--EXPECTF--
Deprecated: Using MYCLASS as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
MyClass
