--TEST--
Returning a Dom\Node from Dom\XPath callback
--EXTENSIONS--
dom
--FILE--
<?php

$dom = Dom\XMLDocument::createFromString('<root/>');
$xpath = new Dom\XPath($dom);
$xpath->registerPhpFunctionNs('urn:x', 'test', fn() => $dom->createElement('foo'));
$xpath->registerNamespace('x', 'urn:x');
$test = $xpath->query('x:test()');
var_dump($test[0]->nodeName);

?>
--EXPECTF--
Deprecated: Calling registerPhpFunctionNs() is deprecated, use the correct casing Dom\XPath::registerPhpFunctionNS() instead in %s on line %d
string(3) "foo"
