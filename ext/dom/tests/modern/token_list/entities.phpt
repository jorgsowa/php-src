--TEST--
TokenList: entities interaction
--EXTENSIONS--
dom
--FILE--
<?php

$dom = DOM\XMLDocument::createFromString(<<<XML
<?xml version="1.0" encoding="UTF-8" ?>
<!DOCTYPE root [
    <!ENTITY ent "foo">
]>
<root class="x&ent;x"/>
XML);
$element = $dom->documentElement;
$list = $element->classList;

var_dump($list);

var_dump($list->contains("xfoox"));
var_dump($list->contains("xx"));
var_dump($list->contains("foo"));

$list->add("test");

echo $dom->saveXML();

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d
object(Dom\TokenList)#3 (2) {
  ["length"]=>
  int(1)
  ["value"]=>
  string(5) "xfoox"
}
bool(true)
bool(false)
bool(false)

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE root [
<!ENTITY ent "foo">
]>
<root class="xfoox test"/>
