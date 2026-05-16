--TEST--
Type declarations with wrong-cased class names emit E_DEPRECATED
--FILE--
<?php
class MyClass {}

function acceptMyClass(MYCLASS $x): MYCLASS { return $x; }

$obj = new MyClass();
$result = acceptMyClass($obj);
echo get_class($result) . "\n";
?>
--EXPECTF--
Deprecated: Using MYCLASS as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
MyClass
