--TEST--
ReflectionProperty::isReadable() and isWritable() with wrong-case scope name emits E_DEPRECATED
--FILE--
<?php
class MyClass {
    public int $pub = 1;
    protected int $prot = 2;
}

$rp = new ReflectionProperty(MyClass::class, "prot");
var_dump($rp->isReadable("MYCLASS"));
var_dump($rp->isWritable("MYCLASS"));
?>
--EXPECTF--
Deprecated: Using MYCLASS as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
bool(true)

Deprecated: Using MYCLASS as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
bool(true)
