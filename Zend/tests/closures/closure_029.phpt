--TEST--
Closure 029: Testing lambda with instanceof operator
--FILE--
<?php

var_dump(function() { } instanceof closure);
var_dump(function(&$x) { } instanceof closure);
var_dump(@function(&$x) use ($y, $z) { } instanceof closure);

?>
--EXPECTF--

Deprecated: Using closure as a class name with incorrect case is deprecated, use the correct casing Closure instead in %s on line %d
bool(true)

Deprecated: Using closure as a class name with incorrect case is deprecated, use the correct casing Closure instead in %s on line %d
bool(true)

Deprecated: Using closure as a class name with incorrect case is deprecated, use the correct casing Closure instead in %s on line %d
bool(true)
