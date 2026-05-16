--TEST--
CSS Selectors - Quirks mode test
--EXTENSIONS--
dom
--FILE--
<?php

require __DIR__ . '/test_utils.inc';

echo "\n=== Document in quirks mode ===\n\n";

$dom = DOM\HTMLDocument::createFromString(<<<HTML
<html>
    <div class="HElLoWorLD"/>
    <div id="hI"/>
</html>
HTML, LIBXML_NOERROR);

test_helper($dom, 'div.helloworld');
test_helper($dom, 'div.HElLoWorLD');
test_helper($dom, '#hi');
test_helper($dom, '#hI');

echo "\n=== Document not in quirks mode ===\n\n";

$dom = DOM\HTMLDocument::createFromString(<<<HTML
<!DOCTYPE html>
<html>
    <div class="HElLoWorLD"/>
    <div id="hI"/>
</html>
HTML, LIBXML_NOERROR);

test_helper($dom, 'div.helloworld');
test_helper($dom, 'div.HElLoWorLD');
test_helper($dom, '#hi');
test_helper($dom, '#hI');

?>
--EXPECTF--
=== Document in quirks mode ===


Deprecated: Using DOM\HTMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\HTMLDocument instead in %s on line %d

Deprecated: Using DOM\ParentNode as a class name with incorrect case is deprecated, use the correct casing Dom\ParentNode instead in %s on line %d
--- Selector: div.helloworld ---

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\HTMLDocument::saveXml() instead in %s on line %d
<div xmlns="http://www.w3.org/1999/xhtml" class="HElLoWorLD">
    <div id="hI">
</div></div>
--- Selector: div.HElLoWorLD ---
<div xmlns="http://www.w3.org/1999/xhtml" class="HElLoWorLD">
    <div id="hI">
</div></div>
--- Selector: #hi ---
<div xmlns="http://www.w3.org/1999/xhtml" id="hI">
</div>
--- Selector: #hI ---
<div xmlns="http://www.w3.org/1999/xhtml" id="hI">
</div>

=== Document not in quirks mode ===


Deprecated: Using DOM\HTMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\HTMLDocument instead in %s on line %d
--- Selector: div.helloworld ---
--- Selector: div.HElLoWorLD ---
<div xmlns="http://www.w3.org/1999/xhtml" class="HElLoWorLD">
    <div id="hI">
</div></div>
--- Selector: #hi ---
--- Selector: #hI ---
<div xmlns="http://www.w3.org/1999/xhtml" id="hI">
</div>
