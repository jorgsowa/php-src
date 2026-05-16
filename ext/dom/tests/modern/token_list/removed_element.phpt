--TEST--
TokenList: operate on removed element
--EXTENSIONS--
dom
--FILE--
<?php

$dom = DOM\XMLDocument::createFromString('<root class="A B C"/>');
$element = $dom->documentElement;
$list = $element->classList;

$element->remove();

var_dump($list);

$list->remove('B');

var_dump($list);

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d
object(Dom\TokenList)#3 (2) {
  ["length"]=>
  int(3)
  ["value"]=>
  string(5) "A B C"
}
object(Dom\TokenList)#3 (2) {
  ["length"]=>
  int(2)
  ["value"]=>
  string(3) "A C"
}
