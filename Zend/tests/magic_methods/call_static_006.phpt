--TEST--
Testing __callStatic
--FILE--
<?php

class foo {
    static function __callstatic($a, $b) {
        var_dump($a);
    }
}

foo::__construct();

?>
--EXPECTF--
Deprecated: Declaring foo::__callstatic() with incorrect case is deprecated, use the correct casing __callStatic() instead in %s on line %d

Fatal error: Uncaught Error: Cannot call constructor in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d
