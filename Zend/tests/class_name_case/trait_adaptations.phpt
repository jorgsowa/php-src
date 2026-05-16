--TEST--
Class and method name with incorrect case is deprecated in trait adaptations (insteadof / as)
--FILE--
<?php
trait HelloTrait {
    public function sayHello(): string { return 'hello'; }
}

trait GreetTrait {
    public function sayHello(): string { return 'greet'; }
}

// insteadof: wrong-cased trait names on both sides of the rule
class A {
    use HelloTrait, GreetTrait {
        HELLOTRAIT::sayHello insteadof greettrait;
    }
}

// as: wrong-cased trait class name and wrong-cased method name
class B {
    use HelloTrait {
        hellotrait::SAYHELLO as renamed;
    }
}

echo (new A())->sayHello() . "\n";
echo (new B())->renamed() . "\n";
?>
--EXPECTF--
Deprecated: Using HELLOTRAIT as a class name with incorrect case is deprecated, use the correct casing HelloTrait instead in %s on line %d

Deprecated: Using greettrait as a class name with incorrect case is deprecated, use the correct casing GreetTrait instead in %s on line %d

Deprecated: Using hellotrait as a class name with incorrect case is deprecated, use the correct casing HelloTrait instead in %s on line %d

Deprecated: Calling SAYHELLO() is deprecated, use the correct casing HelloTrait::sayHello() instead in %s on line %d
hello
hello
