--TEST--
Template cloning
--EXTENSIONS--
dom
--FILE--
<?php
$dom = Dom\HTMLDocument::createFromString('<template>x</template>', LIBXML_NOERROR);
$a = $dom->head->firstChild->cloneNode(false);
echo $dom->saveXML($a), "\n";
echo $dom->saveHTML($a), "\n";
?>
--EXPECTF--
Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\HTMLDocument::saveXml() instead in %s on line %d
<template xmlns="http://www.w3.org/1999/xhtml"></template>

Deprecated: Calling saveHTML() is deprecated, use the correct casing Dom\HTMLDocument::saveHtml() instead in %s on line %d
<template></template>
