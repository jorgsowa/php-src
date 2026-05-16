--TEST--
Using a trait with wrong-case name emits E_DEPRECATED
--FILE--
<?php
trait MyTrait {
    public function hello(): string {
        return "hello";
    }
}

class Child {
    use MYTRAIT;
}

$obj = new Child();
echo $obj->hello() . "\n";
?>
--EXPECTF--
Deprecated: Using MYTRAIT as a class name with incorrect case is deprecated, use the correct casing MyTrait instead in %s on line %d
hello
