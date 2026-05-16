--TEST--
TokenList: Test equality
--EXTENSIONS--
dom
--FILE--
<?php

$dom = DOM\XMLDocument::createFromString('<root class="a b c"><child/></root>');
$element = $dom->documentElement;

var_dump($element->classList === $element->classList);
$list = $element->classList;
var_dump($list === $list);
var_dump($list === $element->classList);

var_dump($list === $element->firstChild->classList);

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d
bool(true)
bool(true)
bool(true)
bool(false)
