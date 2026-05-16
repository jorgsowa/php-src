--TEST--
Bug #66783 (UAF when appending DOMDocument to element)
--EXTENSIONS--
dom
--FILE--
<?php
$doc = new DomDocument;
$doc->loadXML('<root></root>');
$e = $doc->createElement('e');
try {
    $e->appendChild($doc);
} catch (DOMException $ex) {
    echo $ex->getMessage(), PHP_EOL;
}
?>
--EXPECTF--
Deprecated: Using DomDocument as a class name with incorrect case is deprecated, use the correct casing DOMDocument instead in %s on line %d
Hierarchy Request Error
