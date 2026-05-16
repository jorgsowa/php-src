--TEST--
XMLReader: readString basic
--EXTENSIONS--
xmlreader
--FILE--
<?php

$xml = '<?xml version="1.0" encoding="UTF-8"?><books><book>Book1</book><book>Book2</book></books>';

$reader = new XMLReader();
$reader->xml($xml);
$reader->read();
echo $reader->readString();
?>
--EXPECTF--
Deprecated: Calling xml() is deprecated, use the correct casing XMLReader::XML() instead in %s on line %d
Book1Book2
