--TEST--
Test DOM\Element::closest() method: invalid selector
--EXTENSIONS--
dom
--FILE--
<?php

$dom = DOM\XMLDocument::createFromString("<root/>");

try {
  var_dump($dom->documentElement->closest('@invalid'));
} catch (DOMException $e) {
  echo $e->getMessage();
}

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d
Invalid selector (Selectors. Unexpected token: @invalid)
