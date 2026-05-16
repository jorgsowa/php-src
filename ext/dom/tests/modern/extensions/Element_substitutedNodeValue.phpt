--TEST--
Element::$substitutedNodeValue
--EXTENSIONS--
dom
--FILE--
<?php

$dom = Dom\XMLDocument::createFromString('<root/>');

$dom->documentElement->substitutedNodeValue = "&#x31;";
var_dump($dom->documentElement->substitutedNodeValue);
var_dump($dom->documentElement->nodeValue); // Should always be NULL for elements
echo $dom->saveXML(), "\n";

$dom->documentElement->substitutedNodeValue = "&lt;&gt;";
var_dump($dom->documentElement->substitutedNodeValue);
var_dump($dom->documentElement->nodeValue); // Should always be NULL for elements
echo $dom->saveXML(), "\n";

$dom->documentElement->substitutedNodeValue = "";
var_dump($dom->documentElement->substitutedNodeValue);
var_dump($dom->documentElement->nodeValue); // Should always be NULL for elements
echo $dom->saveXML(), "\n";

?>
--EXPECTF--
string(1) "1"
NULL

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root>1</root>
string(2) "<>"
NULL

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root>&lt;&gt;</root>
string(0) ""
NULL

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
%r(<root><\/root>|<root\/>)%r
