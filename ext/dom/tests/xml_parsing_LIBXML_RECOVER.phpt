--TEST--
XML parsing with LIBXML_RECOVER
--EXTENSIONS--
dom
--FILE--
<?php

$dom = new DOMDocument;
$dom->loadXML('<root><child/>', options: LIBXML_RECOVER);
echo $dom->saveXML();

$dom = Dom\XMLDocument::createFromString('<root><child/>', options: LIBXML_RECOVER);
echo $dom->saveXML(), "\n";

?>
--EXPECTF--
Warning: DOMDocument::loadXML(): %s
<?xml version="1.0"?>
<root><child/></root>

Warning: Dom\XMLDocument::createFromString(): %s

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root><child/></root>
