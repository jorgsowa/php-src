--TEST--
Invalid method name in dynamic static call
--FILE--
<?php

class foo {
    static function __callstatic($a, $b) {
        var_dump($a);
    }
}

foo::AaA();

$a = 1;
foo::$a();

?>
--EXPECTF--
Deprecated: Declaring foo::__callstatic() with incorrect case is deprecated, use the correct casing __callStatic() instead in %s on line %d
string(3) "AaA"

Fatal error: Uncaught Error: Method name must be a string in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d
