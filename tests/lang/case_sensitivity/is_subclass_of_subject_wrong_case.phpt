--TEST--
is_subclass_of() and is_a() with wrong-case subject string emit E_DEPRECATED
--FILE--
<?php
class Base {}
class Child extends Base {}

var_dump(is_subclass_of("CHILD", "Base"));
var_dump(is_subclass_of("child", "Base"));
var_dump(is_subclass_of("Child", "Base"));

// is_a() with allow_string=true (third arg)
var_dump(is_a("CHILD", "Base", true));
?>
--EXPECTF--
Deprecated: Using CHILD as a class name with incorrect case is deprecated, use the correct casing Child instead in %s on line %d
bool(true)

Deprecated: Using child as a class name with incorrect case is deprecated, use the correct casing Child instead in %s on line %d
bool(true)
bool(true)

Deprecated: Using CHILD as a class name with incorrect case is deprecated, use the correct casing Child instead in %s on line %d
bool(true)
