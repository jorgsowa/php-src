--TEST--
TokenList: toggle
--EXTENSIONS--
dom
--FILE--
<?php

$dom = DOM\XMLDocument::createFromString('<root class="A B C"/>');
$element = $dom->documentElement;
$list = $element->classList;

echo "--- Toggle A (forced add) ---\n";

var_dump($list->toggle("A", true));

echo $dom->saveXML(), "\n";

echo "--- Toggle A (not forced) ---\n";

var_dump($list->toggle("A"));

echo $dom->saveXML(), "\n";

echo "--- Toggle A (forced remove) ---\n";

var_dump($list->toggle("A", false));

echo $dom->saveXML(), "\n";

echo "--- Toggle B (forced remove) ---\n";

var_dump($list->toggle("B", false));

echo $dom->saveXML(), "\n";

echo "--- Toggle D ---\n";

var_dump($list->toggle("D"));

echo $dom->saveXML(), "\n";

echo "--- Toggle C ---\n";

var_dump($list->toggle("C"));

echo $dom->saveXML(), "\n";

echo "--- Toggle E ---\n";

$list->value = 'E';
$list->toggle('E');

echo $dom->saveXML(), "\n";

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d
--- Toggle A (forced add) ---
bool(true)

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root class="A B C"/>
--- Toggle A (not forced) ---
bool(false)

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root class="B C"/>
--- Toggle A (forced remove) ---
bool(false)

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root class="B C"/>
--- Toggle B (forced remove) ---
bool(false)

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root class="C"/>
--- Toggle D ---
bool(true)

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root class="C D"/>
--- Toggle C ---
bool(false)

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root class="D"/>
--- Toggle E ---

Deprecated: Calling saveXML() is deprecated, use the correct casing Dom\XMLDocument::saveXml() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<root class=""/>
