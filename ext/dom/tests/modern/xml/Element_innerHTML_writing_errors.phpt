--TEST--
Test writing Element::$innerHTML on XML documents - error cases
--EXTENSIONS--
dom
--FILE--
<?php

$dom = DOM\XMLDocument::createFromString(<<<XML
<!DOCTYPE root [
    <!ENTITY foo "content">
]>
<root/>
XML);
$child = $dom->documentElement->appendChild($dom->createElementNS('urn:a', 'child'));
$original = $dom->saveXML();

function test($child, $html) {
    global $dom, $original;
    try {
        $child->innerHTML = $html;
    } catch (DOMException $e) {
        echo $e->getMessage(), "\n";
    }
    var_dump($dom->saveXML() === $original);
}

test($child, '&foo;');
test($child, '</root>');
test($child, '</root><foo/><!--');
test($child, '--></root><!--');
test($child, '<');
test($child, '<!ENTITY foo "content">');

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
XML fragment is not well-formed

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
bool(true)
XML fragment is not well-formed
bool(true)
XML fragment is not well-formed
bool(true)
XML fragment is not well-formed
bool(true)
XML fragment is not well-formed
bool(true)
XML fragment is not well-formed
bool(true)
