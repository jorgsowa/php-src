--TEST--
Testing creation of alias to an internal class
--FILE--
<?php

class_alias('stdclass', 'foo');
$foo = new foo();
var_dump($foo);

?>
--EXPECTF--
Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
object(stdClass)#1 (0) {
}
