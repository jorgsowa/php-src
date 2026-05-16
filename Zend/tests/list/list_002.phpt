--TEST--
Testing full-reference on list()
--FILE--
<?php

error_reporting(E_ALL);

$a = new stdclass;
$b =& $a;

list($a, list($b)) = array($a, array($b));
var_dump($a, $b, $a === $b);

?>
--EXPECTF--

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
object(stdClass)#1 (0) {
}
object(stdClass)#1 (0) {
}
bool(true)
