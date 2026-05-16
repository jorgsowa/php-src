--TEST--
TokenList: change attribute
--EXTENSIONS--
dom
--FILE--
<?php

$dom = DOM\XMLDocument::createFromString('<root class="a b c"><child/></root>');
$element = $dom->documentElement;
$list = $element->classList;

var_dump($list);

$element->attributes[0]->value = 'd';

var_dump($list);

$list->value = 'e f g';

var_dump($list);

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d
object(Dom\TokenList)#3 (2) {
  ["length"]=>
  int(3)
  ["value"]=>
  string(5) "a b c"
}
object(Dom\TokenList)#3 (2) {
  ["length"]=>
  int(1)
  ["value"]=>
  string(1) "d"
}
object(Dom\TokenList)#3 (2) {
  ["length"]=>
  int(3)
  ["value"]=>
  string(5) "e f g"
}
