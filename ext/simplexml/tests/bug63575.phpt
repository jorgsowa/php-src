--TEST--
Bug #63575 (Root elements are not properly cloned)
--EXTENSIONS--
simplexml
--FILE--
<?php
$xml = '<a><b></b></a>';

$o1 = new SimpleXMlElement($xml);
$o2 = clone $o1;

$r = current($o2->xpath('/a'));
$r->addChild('c', new SimpleXMlElement('<c></c>'));

echo $o1->asXML(), PHP_EOL, $o2->asXML();
?>
--EXPECTF--
Deprecated: Using SimpleXMlElement as a class name with incorrect case is deprecated, use the correct casing SimpleXMLElement instead in %s on line %d

Deprecated: Using SimpleXMlElement as a class name with incorrect case is deprecated, use the correct casing SimpleXMLElement instead in %s on line %d
<?xml version="1.0"?>
<a><b/></a>

<?xml version="1.0"?>
<a><b/><c/></a>
