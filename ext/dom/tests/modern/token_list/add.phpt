--TEST--
TokenList: add
--EXTENSIONS--
dom
--FILE--
<?php

$dom = DOM\XMLDocument::createFromString('<root/>');
$list = $dom->documentElement->classList;

$list->add();
$list->add('a', 'b');
$list->add('c');

$str = 'd';
$ref =& $str;

$list->add($ref);

echo $dom->saveXML(), "\n";

$list->value = '';
$list->add('e');

echo $dom->saveXML(), "\n";

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root class="a b c d"/>

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root class="e"/>
