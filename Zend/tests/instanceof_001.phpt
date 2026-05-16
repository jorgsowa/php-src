--TEST--
Testing instanceof operator with several operators
--FILE--
<?php

$a = new stdClass;
var_dump($a instanceof stdClass);

var_dump(new stdCLass instanceof stdClass);

$b = function() { return new stdClass; };
var_dump($b() instanceof stdClass);

$c = array(new stdClass);
var_dump($c[0] instanceof stdClass);

var_dump(@$inexistent instanceof stdClass);

?>
--EXPECTF--
bool(true)

Deprecated: Using stdCLass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
bool(true)
bool(true)
bool(true)
bool(false)
