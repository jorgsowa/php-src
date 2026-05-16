--TEST--
Bug #75451 (Assertion fails while foreach on empty xpath query)
--EXTENSIONS--
dom
--FILE--
<?php
$dom = new DOMDocument();
$dom->loadXML('<root><child/></root>');
$xpath = new DOMXpath($dom);
foreach($xpath->query('/root/noexist') as $child) {
    var_dump($child);
}
?>
okey
--EXPECTF--
Deprecated: Using DOMXpath as a class name with incorrect case is deprecated, use the correct casing DOMXPath instead in %s on line %d
okey
