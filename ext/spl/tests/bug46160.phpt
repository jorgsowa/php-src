--TEST--
Bug #46160 (SPL - Memory leak when exception is throwed in offsetSet method)
--FILE--
<?php

try {
    $x = new splqueue;
    $x->offsetSet(0, 0);
} catch (Exception $e) { }

?>
DONE
--EXPECTF--
Deprecated: Using splqueue as a class name with incorrect case is deprecated, use the correct casing SplQueue instead in %s on line %d
DONE
