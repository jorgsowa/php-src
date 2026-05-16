--TEST--
Testing __callstatic declaration with wrong modifier
--FILE--
<?php

class a {
    static protected function __callstatic($a, $b) {
    }
}

?>
--EXPECTF--
Deprecated: Declaring a::__callstatic() with incorrect case is deprecated, use the correct casing __callStatic() instead in %s on line %d

Warning: The magic method a::__callstatic() must have public visibility in %s on line %d
