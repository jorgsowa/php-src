--TEST--
CSS Selectors - Pseudo classes: empty
--EXTENSIONS--
dom
--FILE--
<?php

require __DIR__ . '/test_utils.inc';

$dom = DOM\XMLDocument::createFromString(<<<XML
<container>
    <div class="empty">
        <p>Element with no content:</p>
        <div></div>

        <p>Element with comment:</p>
        <div><!-- Simple Comment --></div>

        <p>Element with PI:</p>
        <div><?foo?></div>

        <p>Element with CDATA:</p>
        <div><![CDATA[foo]]></div>

        <p>Element with nested empty element:</p>
        <div><p></p></div>
    </div>
</container>
XML);

test_helper($dom, '.empty > div:empty');

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d

Deprecated: Using DOM\ParentNode as a class name with incorrect case is deprecated, use the correct casing Dom\ParentNode instead in %s on line %d
--- Selector: .empty > div:empty ---

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<div/>
<div><!-- Simple Comment --></div>
<div><?foo ?></div>
