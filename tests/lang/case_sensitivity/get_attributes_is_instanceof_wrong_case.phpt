--TEST--
ReflectionClass::getAttributes() with wrong-case class name and IS_INSTANCEOF emits E_DEPRECATED
--FILE--
<?php
#[Attribute(Attribute::TARGET_CLASS)]
class MyAttr {}

#[MyAttr]
class Foo {}

$rc = new ReflectionClass(Foo::class);

$attrs = $rc->getAttributes("MYATTR", ReflectionAttribute::IS_INSTANCEOF);
echo "count: " . count($attrs) . "\n";

$attrs2 = $rc->getAttributes("myattr", ReflectionAttribute::IS_INSTANCEOF);
echo "count: " . count($attrs2) . "\n";
?>
--EXPECTF--
Deprecated: Using MYATTR as a class name with incorrect case is deprecated, use the correct casing MyAttr instead in %s on line %d
count: 1

Deprecated: Using myattr as a class name with incorrect case is deprecated, use the correct casing MyAttr instead in %s on line %d
count: 1
