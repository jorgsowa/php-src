--TEST--
TokenList: remove
--EXTENSIONS--
dom
--FILE--
<?php

$dom = DOM\XMLDocument::createFromString('<root class="test1    test2"/>');
$list = $dom->documentElement->classList;

$list->remove();
$list->remove('test1');

echo $dom->saveXML(), "\n";

$list->remove('nope');

echo $dom->saveXML(), "\n";

$list->remove('test2');

echo $dom->saveXML(), "\n";

$list->value = 'test3 test4';
$list->remove('test4');

echo $dom->saveXML(), "\n";

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root class="test2"/>

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root class="test2"/>

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root class=""/>

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root class="test3"/>
