--TEST--
CSS Selectors - Pseudo classes: only-of-type
--EXTENSIONS--
dom
--FILE--
<?php

require __DIR__ . '/test_utils.inc';

$dom = DOM\XMLDocument::createFromString(<<<XML
<container>
    <div class="only-of-type1">
        <p>Alone</p>
    </div>
    <div class="only-of-type2">
        <p>With 2</p>
        <p>With 2</p>
    </div>
    <div class="only-of-type3">
        <p>With 2</p>
        <div/>
        <p>With 2</p>
    </div>
</container>
XML);

test_helper($dom, '.only-of-type1 p:only-of-type');
test_helper($dom, '.only-of-type2 p:only-of-type');
test_helper($dom, '.only-of-type3 p:only-of-type');

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d

Deprecated: Using DOM\ParentNode as a class name with incorrect case is deprecated, use the correct casing Dom\ParentNode instead in %s on line %d
--- Selector: .only-of-type1 p:only-of-type ---

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<p>Alone</p>
--- Selector: .only-of-type2 p:only-of-type ---
--- Selector: .only-of-type3 p:only-of-type ---
