--TEST--
$innerHTML cache invalidation
--EXTENSIONS--
dom
--FILE--
<?php

$dom = Dom\XMLDocument::createFromString('<root><a/><a/><a/></root>');
$els = $dom->getElementsByTagName('a');
var_dump($els[0]->tagName);

$dom->documentElement->innerHTML = '<b/>';

echo $dom->saveXML(), "\n";
var_dump($els);
var_dump($els[0]?->tagName);

?>
--EXPECTF--
string(1) "a"

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root><b/></root>
object(Dom\HTMLCollection)#2 (1) {
  ["length"]=>
  int(0)
}
NULL
