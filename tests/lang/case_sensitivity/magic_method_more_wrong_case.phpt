--TEST--
Declaring __sleep, __wakeup, __serialize, __unserialize, __set_state with wrong case emits E_DEPRECATED
--FILE--
<?php
class Foo {
    public function __SLEEP() { return []; }
    public function __WAKEUP() {}
    public function __SERIALIZE() { return []; }
    public function __UNSERIALIZE($data) {}
    public static function __SET_STATE($array) { return new self; }
}
echo "done\n";
?>
--EXPECTF--
Deprecated: Declaring Foo::__SLEEP() with incorrect case is deprecated, use the correct casing __sleep() instead in %s on line %d

Deprecated: Declaring Foo::__WAKEUP() with incorrect case is deprecated, use the correct casing __wakeup() instead in %s on line %d

Deprecated: Declaring Foo::__SERIALIZE() with incorrect case is deprecated, use the correct casing __serialize() instead in %s on line %d

Deprecated: Declaring Foo::__UNSERIALIZE() with incorrect case is deprecated, use the correct casing __unserialize() instead in %s on line %d

Deprecated: Declaring Foo::__SET_STATE() with incorrect case is deprecated, use the correct casing __set_state() instead in %s on line %d
done
