--TEST--
instanceof with wrong-case class name emits E_DEPRECATED
--FILE--
<?php
class MyException extends Exception {}

$ex = new MyException();

var_dump($ex instanceof MYEXCEPTION);
var_dump($ex instanceof myexception);
var_dump($ex instanceof MyException);
?>
--EXPECTF--
Deprecated: Using MYEXCEPTION as a class name with incorrect case is deprecated, use the correct casing MyException instead in %s on line %d
bool(true)

Deprecated: Using myexception as a class name with incorrect case is deprecated, use the correct casing MyException instead in %s on line %d
bool(true)
bool(true)
