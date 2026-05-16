--TEST--
CSS Selectors - Pseudo classes: root
--EXTENSIONS--
dom
--FILE--
<?php

require __DIR__ . '/test_utils.inc';

$dom = DOM\XMLDocument::createFromString(<<<XML
<container/>
XML);

test_helper($dom, ':root', true);
$fragment = $dom->createDocumentFragment();
$fragment->appendXML('<div><p></p></div>');
test_helper($fragment, ':root', true);
test_helper($dom->createElement("foo"), ':root', true);

test_helper($dom->createElement("not-a-root"), ":root", true);

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d

Deprecated: Using DOM\ParentNode as a class name with incorrect case is deprecated, use the correct casing Dom\ParentNode instead in %s on line %d
--- Selector: :root ---
container

Deprecated: Calling appendXML() is deprecated, use the correct casing Dom\DocumentFragment::appendXml() instead in %s on line %d
--- Selector: :root ---
div
--- Selector: :root ---
--- Selector: :root ---
