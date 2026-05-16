--TEST--
<template> element nesting
--EXTENSIONS--
dom
--FILE--
<?php

$html = <<<HTML
<!DOCTYPE html>
<html>
    <body>
        <template>foo<template>bar</template></template>
    </body>
</html>
HTML;
$dom = Dom\HTMLDocument::createFromString($html);
$template = $dom->body->firstElementChild;
var_dump($template->innerHTML);
echo $dom->saveXML();

?>
--EXPECTF--
string(27) "foo<template>bar</template>"

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\HTMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml"><head></head><body>
        <template>foo<template>bar</template></template>
    
</body></html>
