--TEST--
TokenList: add errors
--EXTENSIONS--
dom
--FILE--
<?php

$dom = DOM\XMLDocument::createFromString("<root/>");
$list = $dom->documentElement->classList;

try {
    $list->add("");
} catch (DOMException $e) {
    echo $e->getMessage(), "\n";
}
try {
    $list->add("  ");
} catch (DOMException $e) {
    echo $e->getMessage(), "\n";
}
try {
    $list->add("\0");
} catch (ValueError $e) {
    echo $e->getMessage(), "\n";
}
try {
    $list->add(0);
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}

echo $dom->saveXML(), "\n";

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d
The empty string is not a valid token
The token must not contain any ASCII whitespace
Dom\TokenList::add(): Argument #1 must not contain any null bytes
Dom\TokenList::add(): Argument #1 must be of type string, int given

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root/>
