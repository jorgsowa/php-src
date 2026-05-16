--TEST--
CSS Selectors - Pseudo classes: nth-child
--EXTENSIONS--
dom
--FILE--
<?php

require __DIR__ . '/test_utils.inc';

$dom = DOM\XMLDocument::createFromString(<<<XML
<container>
    <h2>1</h2>
    <h2>2</h2>
    <h2>3</h2>
    <h2>4</h2>
    <h2>5</h2>
</container>
XML);

test_helper($dom, 'h2:nth-of-type(n+2):nth-last-of-type(n+2)');
test_helper($dom, 'h2:not(:first-of-type):not(:last-of-type)'); // Equivalent to the above
test_helper($dom, 'h2:nth-child(2)');
test_helper($dom, 'h2:nth-last-child(2)');
test_helper($dom, 'h2:nth-child(2n + 1)');
test_helper($dom, 'h2:nth-last-child(2n + 1)');
test_helper($dom, 'h2:nth-child(3n - 2)');
test_helper($dom, 'h2:nth-last-child(3n - 2)');

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d

Deprecated: Using DOM\ParentNode as a class name with incorrect case is deprecated, use the correct casing Dom\ParentNode instead in %s on line %d
--- Selector: h2:nth-of-type(n+2):nth-last-of-type(n+2) ---

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<h2>2</h2>
<h2>3</h2>
<h2>4</h2>
--- Selector: h2:not(:first-of-type):not(:last-of-type) ---
<h2>2</h2>
<h2>3</h2>
<h2>4</h2>
--- Selector: h2:nth-child(2) ---
<h2>2</h2>
--- Selector: h2:nth-last-child(2) ---
<h2>4</h2>
--- Selector: h2:nth-child(2n + 1) ---
<h2>1</h2>
<h2>3</h2>
<h2>5</h2>
--- Selector: h2:nth-last-child(2n + 1) ---
<h2>1</h2>
<h2>3</h2>
<h2>5</h2>
--- Selector: h2:nth-child(3n - 2) ---
<h2>1</h2>
<h2>4</h2>
--- Selector: h2:nth-last-child(3n - 2) ---
<h2>2</h2>
<h2>5</h2>
