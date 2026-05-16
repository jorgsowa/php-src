--TEST--
"Nested" list()
--FILE--
<?php

list($a, list($b)) = array(new stdclass, array(new stdclass));
var_dump($a, $b);
[$a, [$b]] = array(new stdclass, array(new stdclass));
var_dump($a, $b);

?>
--EXPECTF--

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
object(stdClass)#1 (0) {
}
object(stdClass)#2 (0) {
}

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
object(stdClass)#3 (0) {
}
object(stdClass)#4 (0) {
}
