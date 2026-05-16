--TEST--
Test setting innerHTML on a prefixed element
--EXTENSIONS--
dom
--FILE--
<?php
$dom = Dom\XMLDocument::createFromString('<x:root xmlns:x="urn:x"/>');
$dom->documentElement->innerHTML = '<child><x:subchild/></child>';
echo $dom->saveXML();
?>
--EXPECTF--
Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<x:root xmlns:x="urn:x"><child><x:subchild/></child></x:root>
