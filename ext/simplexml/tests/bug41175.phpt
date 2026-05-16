--TEST--
Bug #41175 (addAttribute() fails to add an attribute with an empty value)
--EXTENSIONS--
simplexml
--FILE--
<?php

$xml = new SimpleXmlElement("<img></img>");
$xml->addAttribute("src", "foo");
$xml->addAttribute("alt", "");
echo $xml->asXML();

?>
--EXPECTF--
Deprecated: Using SimpleXmlElement as a class name with incorrect case is deprecated, use the correct casing SimpleXMLElement instead in %s on line %d
<?xml version="1.0"?>
<img src="foo" alt=""/>
