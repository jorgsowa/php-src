--TEST--
<template> element contents do not participate in DOM
--EXTENSIONS--
dom
--FILE--
<?php

$html = <<<HTML
<!DOCTYPE html>
<html>
    <body>
        <template>a<div>foo</div>b</template>
    </body>
</html>
HTML;
$dom = Dom\HTMLDocument::createFromString($html);
$template = $dom->body->firstElementChild;

echo "=== Manipulation ===\n";

echo "First child of template: ";
var_dump($template->firstChild?->nodeName);
$template->append($dom->createElement('invisible'));

echo "First child of template after appending: ";
var_dump($template->firstChild->nodeName);
$template->innerHTML = $template->innerHTML;
echo "Inner HTML after idempotent modification: ";
var_dump($template->innerHTML);
echo "Selector should not find div element in shadow DOM: ";
var_dump($template->querySelector('div'));

echo "XPath should not find div element in shadow DOM:\n";
$xpath = new Dom\XPath($dom);
var_dump($xpath->query('//div'));

echo "=== HTML serialization ===\n";
echo $dom->saveHTML(), "\n";
echo "=== HTML serialization of <template> ===\n";
echo $dom->saveHTML($template), "\n";
echo "=== XML serialization ===\n";
echo $dom->saveXML(), "\n";
echo "=== XML serialization of <template> ===\n";
echo $dom->saveXML($template), "\n";

// Should not crash
$template->remove();
unset($template);

echo "=== Creating a new template should not leak the old contents ===\n";
$template = $dom->createElement('template');
var_dump($template->innerHTML);

?>
--EXPECTF--
=== Manipulation ===
First child of template: NULL
First child of template after appending: string(9) "INVISIBLE"
Inner HTML after idempotent modification: string(16) "a<div>foo</div>b"
Selector should not find div element in shadow DOM: NULL
XPath should not find div element in shadow DOM:
object(Dom\NodeList)#4 (1) {
  ["length"]=>
  int(0)
}
=== HTML serialization ===

Deprecated: Calling saveHTML() is deprecated, use the correct casing Dom\HTMLDocument::saveHtml() instead in %s on line %d
<!DOCTYPE html><html><head></head><body>
        <template>a<div>foo</div>b</template>
    
</body></html>
=== HTML serialization of <template> ===

Deprecated: Calling saveHTML() is deprecated, use the correct casing Dom\HTMLDocument::saveHtml() instead in %s on line %d
<template>a<div>foo</div>b</template>
=== XML serialization ===

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\HTMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml"><head></head><body>
        <template>a<div>foo</div>b</template>
    
</body></html>
=== XML serialization of <template> ===

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\HTMLDocument::saveXml() instead in %s on line %d
<template xmlns="http://www.w3.org/1999/xhtml">a<div>foo</div>b</template>
=== Creating a new template should not leak the old contents ===
string(0) ""
