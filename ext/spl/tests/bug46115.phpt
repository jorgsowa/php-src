--TEST--
Bug #46115 (Memory leak when calling a method using Reflection)
--FILE--
<?php
$h = new RecursiveArrayIterator(array());
$x = new reflectionmethod('RecursiveArrayIterator', 'asort');
$z = $x->invoke($h);
?>
DONE
--EXPECTF--
Deprecated: Using reflectionmethod as a class name with incorrect case is deprecated, use the correct casing ReflectionMethod instead in %s on line %d
DONE
