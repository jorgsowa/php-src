--TEST--
XPath: basic types evaluation
--EXTENSIONS--
dom
--FILE--
<?php

$dom = new DOMDocument();
$dom->loadHTML('<p align="center">foo</p>');
$xpath = new DOMXpath($dom);
var_dump($xpath->evaluate("count(//p) > 0"));
var_dump($xpath->evaluate("string(//p/@align)"));

?>
--EXPECTF--
Deprecated: Using DOMXpath as a class name with incorrect case is deprecated, use the correct casing DOMXPath instead in %s on line %d
bool(true)
string(6) "center"
