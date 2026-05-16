--TEST--
CSS Selectors - Pseudo classes: first/last child
--EXTENSIONS--
dom
--FILE--
<?php

require __DIR__ . '/test_utils.inc';

$dom = DOM\XMLDocument::createFromString(<<<XML
<container>
    <?foo?>
    <!--bar-->
    <first />
    &amp;
    <last/>
    <![CDATA[skip me]]>
</container>
XML);

test_helper($dom, 'container > *:first-child');
test_helper($dom, 'container > *:last-child');

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d

Deprecated: Using DOM\ParentNode as a class name with incorrect case is deprecated, use the correct casing Dom\ParentNode instead in %s on line %d
--- Selector: container > *:first-child ---

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<first/>
--- Selector: container > *:last-child ---
<last/>
