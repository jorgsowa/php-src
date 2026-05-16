--TEST--
CSS Selectors - Pseudo classes: placeholder-shown
--EXTENSIONS--
dom
--FILE--
<?php

require __DIR__ . '/test_utils.inc';

$dom = DOM\XMLDocument::createFromString(<<<XML
<html xmlns="http://www.w3.org/1999/xhtml">
    <input type="text" placeholder="" />
    <textarea placeholder="" />
    <input xmlns="" type="text" placeholder="" />
    <textarea xmlns="" placeholder="" />
    <input type="text" />
    <textarea />
</html>
XML);

test_helper($dom, ':placeholder-shown');

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d

Deprecated: Using DOM\ParentNode as a class name with incorrect case is deprecated, use the correct casing Dom\ParentNode instead in %s on line %d
--- Selector: :placeholder-shown ---

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<input xmlns="http://www.w3.org/1999/xhtml" type="text" placeholder="" />
<textarea xmlns="http://www.w3.org/1999/xhtml" placeholder=""></textarea>
