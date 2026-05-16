--TEST--
ReflectionAttribute::newInstance() with wrong-case attribute name emits E_DEPRECATED
--FILE--
<?php
#[Attribute]
class MyAttr {
    public function __construct(public int $value = 0) {}
}

#[MYATTR(1)]
class Foo {}

$rc = new ReflectionClass(Foo::class);
$attrs = $rc->getAttributes();
foreach ($attrs as $attr) {
    $instance = $attr->newInstance();
    echo get_class($instance) . "\n";
    echo $instance->value . "\n";
}
?>
--EXPECTF--
Deprecated: Using MYATTR as a class name with incorrect case is deprecated, use the correct casing MyAttr instead in %s on line %d
MyAttr
1
