--TEST--
CSS Selectors - Pseudo classes: has
--EXTENSIONS--
dom
--FILE--
<?php

require __DIR__ . '/test_utils.inc';

$dom = DOM\XMLDocument::createFromString(<<<XML
<container>
    <div>
        <p class="foo"/>
    </div>
    <div>
        <p/>
    </div>
</container>
XML);

test_helper($dom, 'div:has(p.foo)');
test_helper($dom, 'div:has(:not(p.foo))');

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d

Deprecated: Using DOM\ParentNode as a class name with incorrect case is deprecated, use the correct casing Dom\ParentNode instead in %s on line %d
--- Selector: div:has(p.foo) ---

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<div>
        <p class="foo"/>
    </div>
--- Selector: div:has(:not(p.foo)) ---
<div>
        <p/>
    </div>
