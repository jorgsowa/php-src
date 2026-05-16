--TEST--
Bug #41582 (SimpleXML crashes when accessing newly created element)
--EXTENSIONS--
simplexml
--FILE--
<?php

$xml = new SimpleXMLElement('<?xml version="1.0" standalone="yes"?>
<collection></collection>');

$xml->movie[]->characters->character[0]->name = 'Miss Coder';

echo($xml->asXml());

?>
--EXPECTF--
Deprecated: Calling asXml() is deprecated, use the correct casing SimpleXMLElement::asXML() instead in %s on line %d
<?xml version="1.0" standalone="yes"?>
<collection><movie><characters><character><name>Miss Coder</name></character></characters></movie></collection>
