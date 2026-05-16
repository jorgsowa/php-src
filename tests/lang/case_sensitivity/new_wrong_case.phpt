--TEST--
Class instantiation via new with wrong-case class name emits E_DEPRECATED
--FILE--
<?php
class ProductService {}

$obj = new PRODUCTSERVICE();
echo get_class($obj) . "\n";

$obj2 = new productservice();
echo get_class($obj2) . "\n";

$obj3 = new ProductService();
echo get_class($obj3) . "\n";
?>
--EXPECTF--
Deprecated: Using PRODUCTSERVICE as a class name with incorrect case is deprecated, use the correct casing ProductService instead in %s on line %d
ProductService

Deprecated: Using productservice as a class name with incorrect case is deprecated, use the correct casing ProductService instead in %s on line %d
ProductService
ProductService
