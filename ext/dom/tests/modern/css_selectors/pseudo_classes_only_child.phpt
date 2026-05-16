--TEST--
CSS Selectors - Pseudo classes: only-child
--EXTENSIONS--
dom
--FILE--
<?php

require __DIR__ . '/test_utils.inc';

$dom = DOM\XMLDocument::createFromString(<<<XML
<container>
    <div class="only-child1">
        <p>Lonely</p>
    </div>
    <div class="only-child2">
        <p>With 2</p>
        <p>With 2</p>
    </div>
</container>
XML);

test_helper($dom, '.only-child1 p:only-child');
test_helper($dom, '.only-child2 p:only-child');

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d

Deprecated: Using DOM\ParentNode as a class name with incorrect case is deprecated, use the correct casing Dom\ParentNode instead in %s on line %d
--- Selector: .only-child1 p:only-child ---

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<p>Lonely</p>
--- Selector: .only-child2 p:only-child ---
