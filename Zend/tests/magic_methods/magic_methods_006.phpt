--TEST--
Testing __callstatic declaration in interface with missing the 'static' modifier
--FILE--
<?php

interface a {
    function __callstatic($a, $b);
}

?>
--EXPECTF--
Deprecated: Declaring a::__callstatic() with incorrect case is deprecated, use the correct casing __callStatic() instead in %s on line %d

Fatal error: Method a::__callstatic() must be static in %s on line %d
