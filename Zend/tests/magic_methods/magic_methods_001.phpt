--TEST--
Testing several magic methods
--FILE--
<?php

class foo {
    function __unset($a) {
        print "unset\n";
        var_dump($a);
    }

    public function __call($a, $b) {
        print "call\n";
        var_dump($a);
    }
    function __clone() {
        print "clone\n";
    }
    static public function __callstatic($a, $b) {
        print "callstatic\n";
    }

    public function __tostring() {
        return 'foo';
    }
}


$a = new foo;

$a->sdfdsa();

$a::test();

clone $a;

var_dump((string)$a);

unset($a->a);

?>
--EXPECTF--
Deprecated: Declaring foo::__callstatic() with incorrect case is deprecated, use the correct casing __callStatic() instead in %s on line %d

Deprecated: Declaring foo::__tostring() with incorrect case is deprecated, use the correct casing __toString() instead in %s on line %d
call
string(6) "sdfdsa"
callstatic
clone
string(3) "foo"
unset
string(1) "a"
