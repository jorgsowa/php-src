--TEST--
TokenList: getIterator
--EXTENSIONS--
dom
--FILE--
<?php

$dom = DOM\XMLDocument::createFromString('<root class="A B C"/>');
$element = $dom->documentElement;
$list = $element->classList;

$it = $list->getIterator();
var_dump($it);

var_dump($it->key(), $it->current());
$it->next();
var_dump($it->key(), $it->current());
$it->next();
var_dump($it->key(), $it->current());
$it->next();
var_dump($it->key(), $it->current());

$it->rewind();
var_dump($it->key(), $it->current());

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d
object(InternalIterator)#5 (0) {
}
int(0)
string(1) "A"
int(1)
string(1) "B"
int(2)
string(1) "C"
int(3)
NULL
int(0)
string(1) "A"
