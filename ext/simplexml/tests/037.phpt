--TEST--
SimpleXML: implement Countable
--EXTENSIONS--
simplexml
--FILE--
<?php

$str = '<xml></xml>';
$sxe = new SimpleXmlElement($str);
var_dump($sxe instanceof Countable);
?>
--EXPECTF--
Deprecated: Using SimpleXmlElement as a class name with incorrect case is deprecated, use the correct casing SimpleXMLElement instead in %s on line %d
bool(true)
