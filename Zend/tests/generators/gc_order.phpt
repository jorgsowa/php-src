--TEST--
Close generator in dtor to avoid freeing order issues
--FILE--
<?php

$gen = function() {
    yield;
    throw new Exception; // Just to create a live range
};
$a = new stdclass;
$a->a = $a;
$a->gen = $gen();

?>
===DONE===
--EXPECTF--

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
===DONE===
