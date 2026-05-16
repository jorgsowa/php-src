--TEST--
Testing callback formats within class method
--FILE--
<?php

class foo {
    public function test() {
        call_user_func(array('FOO', 'ABC'));
        call_user_func(array($this, 'ABC'));
        foo::XYZ();
        self::WWW();
        call_user_func('FOO::ABC');
    }
    function __call($a, $b) {
        print "__call:\n";
        var_dump($a);
    }
    static public function __callStatic($a, $b) {
        print "__callstatic:\n";
        var_dump($a);
    }
}

$x = new foo;

$x->test();

$x::A();

foo::B();

$f = 'FOO';

$f::C();

$f::$f();

foo::$f();

?>
--EXPECTF--
__call:
string(3) "ABC"
__call:
string(3) "ABC"
__call:
string(3) "XYZ"
__call:
string(3) "WWW"
__call:
string(3) "ABC"
__callstatic:
string(1) "A"
__callstatic:
string(1) "B"

Deprecated: Using FOO as a class name with incorrect case is deprecated, use the correct casing foo instead in %s on line %d
__callstatic:
string(1) "C"

Deprecated: Using FOO as a class name with incorrect case is deprecated, use the correct casing foo instead in %s on line %d
__callstatic:
string(3) "FOO"
__callstatic:
string(3) "FOO"
