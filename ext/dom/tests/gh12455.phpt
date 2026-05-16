--TEST--
GH-12455 (Namespace prefixes reused incorrectly depending on libxml2 version)
--EXTENSIONS--
dom
--FILE--
<?php

$doc = new DOMDocument();
$element = $doc->createElementNS('http://test', 'a:x');
$doc->appendChild($element);
$element1 = $doc->createElementNS('http://test', 'b:y');
$element->appendChild($element1);
$element1->appendChild($doc->createElementNS('http://test', 'b:z'));
echo $doc->saveXml();

$xpath = new DOMXPath($doc);
$xpath->registerNodeNamespaces = true;
$xpath->registerNamespace('b', 'http://test');

$elements = $xpath->query('//b:z');
foreach ($elements as $e) {
    var_dump($e->nodeName);
}

$elements = $xpath->query('//*[name()="b:z"]');
echo $elements->length;

?>
--EXPECTF--
Deprecated: Calling saveXml() is deprecated, use the correct casing DOMDocument::saveXML() instead in %s on line %d
<?xml version="1.0"?>
<a:x xmlns:a="http://test"><b:y xmlns:b="http://test"><b:z/></b:y></a:x>
string(3) "b:z"
1
