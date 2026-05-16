--TEST--
ReflectionClass::getProperty() with fully-qualified name and wrong-case class emits E_DEPRECATED
--FILE--
<?php
class Base {
    protected int $value = 0;
}

class Child extends Base {}

$rc = new ReflectionClass(Child::class);

// Correct case — no deprecation
$rp = $rc->getProperty("Base::value");
echo $rp->getName() . "\n";

// Wrong case — E_DEPRECATED
$rp2 = $rc->getProperty("BASE::value");
echo $rp2->getName() . "\n";
?>
--EXPECTF--
value

Deprecated: Using BASE as a class name with incorrect case is deprecated, use the correct casing Base instead in %s on line %d
value
