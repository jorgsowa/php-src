--TEST--
Bug #37277 (cloning Dom Documents or Nodes does not work)
--EXTENSIONS--
dom
--FILE--
<?php
$dom1 = new DomDocument('1.0', 'UTF-8');

$xml = '<foo />';
$dom1->loadXml($xml);

$node = clone $dom1->documentElement;

$dom2 = new DomDocument('1.0', 'UTF-8');
$dom2->appendChild($dom2->importNode($node->cloneNode(true), TRUE));

print $dom2->saveXML();


?>
--EXPECTF--
Deprecated: Using DomDocument as a class name with incorrect case is deprecated, use the correct casing DOMDocument instead in %s on line %d

Deprecated: Calling loadXml() is deprecated, use the correct casing DOMDocument::loadXML() instead in %s on line %d

Deprecated: Using DomDocument as a class name with incorrect case is deprecated, use the correct casing DOMDocument instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<foo/>
