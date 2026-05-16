--TEST--
TokenList: debug
--EXTENSIONS--
dom
--FILE--
<?php

$dom = DOM\XMLDocument::createFromString('<root class="a b c"><child/></root>');
$element = $dom->documentElement;
var_dump($element->classList);

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d
object(Dom\TokenList)#3 (2) {
  ["length"]=>
  int(3)
  ["value"]=>
  string(5) "a b c"
}
