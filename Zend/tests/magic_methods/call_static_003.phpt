--TEST--
Testing method name case
--FILE--
<?php

class Foo {
    public function __call($a, $b) {
        print "nonstatic\n";
        var_dump($a);
    }
    static public function __callStatic($a, $b) {
        print "static\n";
        var_dump($a);
    }
    public function test() {
        $this->fOoBaR();
        self::foOBAr();
        $this::fOOBAr();
    }
}

$a = new Foo;
$a->test();
$a::bAr();
foo::BAZ();

?>
--EXPECTF--
nonstatic
string(6) "fOoBaR"
nonstatic
string(6) "foOBAr"
nonstatic
string(6) "fOOBAr"
static
string(3) "bAr"

Deprecated: Using foo as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
static
string(3) "BAZ"
