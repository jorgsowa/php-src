--TEST--
class_parents(), class_implements(), class_uses() with wrong-case class name emits E_DEPRECATED
--FILE--
<?php
class Base {}
class Child extends Base {}
interface MyInterface {}
class Impl implements MyInterface {}
trait MyTrait {}
class WithTrait { use MyTrait; }

$parents = class_parents("CHILD");
echo implode(", ", array_keys($parents)) . "\n";

$interfaces = class_implements("IMPL");
echo implode(", ", array_keys($interfaces)) . "\n";

$traits = class_uses("WITHTRAIT");
echo implode(", ", array_keys($traits)) . "\n";
?>
--EXPECTF--
Deprecated: Using CHILD as a class name with incorrect case is deprecated, use the correct casing Child instead in %s on line %d
Base

Deprecated: Using IMPL as a class name with incorrect case is deprecated, use the correct casing Impl instead in %s on line %d
MyInterface

Deprecated: Using WITHTRAIT as a class name with incorrect case is deprecated, use the correct casing WithTrait instead in %s on line %d
MyTrait
