--TEST--
new with an arbitrary expression
--FILE--
<?php

$class = 'class';
var_dump(new ('std'.$class));
var_dump(new ('std'.$class)());
$obj = new stdClass;
var_dump($obj instanceof ('std'.$class));

?>
--EXPECTF--

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
object(stdClass)#1 (0) {
}

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
object(stdClass)#1 (0) {
}

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
bool(true)
