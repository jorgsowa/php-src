--TEST--
TokenList: contains empty
--EXTENSIONS--
dom
--FILE--
<?php

$dom = DOM\XMLDocument::createFromString('<root/>');
$element = $dom->documentElement;
$list = $element->classList;

var_dump($list->contains('x'));

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d
bool(false)
