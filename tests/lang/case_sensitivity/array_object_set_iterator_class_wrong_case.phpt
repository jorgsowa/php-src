--TEST--
ArrayObject::setIteratorClass() with wrong-case class name emits E_DEPRECATED
--FILE--
<?php
class MyArrayIterator extends ArrayIterator {}

$ao = new ArrayObject([1, 2, 3]);
$ao->setIteratorClass("MYARRAYITERATOR");
echo $ao->getIteratorClass() . "\n";

$ao->setIteratorClass("myarrayiterator");
echo $ao->getIteratorClass() . "\n";

$ao->setIteratorClass("MyArrayIterator");
echo $ao->getIteratorClass() . "\n";
?>
--EXPECTF--
Deprecated: Using MYARRAYITERATOR as a class name with incorrect case is deprecated, use the correct casing MyArrayIterator instead in %s on line %d
MyArrayIterator

Deprecated: Using myarrayiterator as a class name with incorrect case is deprecated, use the correct casing MyArrayIterator instead in %s on line %d
MyArrayIterator
MyArrayIterator
