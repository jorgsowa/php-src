--TEST--
Static method called with wrong case emits E_DEPRECATED
--FILE--
<?php
class Bar {
    public static function staticMethod() { return "static ok"; }
}
echo Bar::STATICMETHOD() . "\n";
echo Bar::staticMethod() . "\n";
?>
--EXPECTF--
Deprecated: Calling STATICMETHOD() is deprecated, use the correct casing Bar::staticMethod() instead in %s on line %d
static ok
static ok
