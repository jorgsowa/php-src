--TEST--
Declaring magic methods with wrong case emits E_DEPRECATED
--FILE--
<?php
class Foo {
    public function __CONSTRUCT() {}
    public function __DESTRUCT() {}
    public function __tostring() { return "foo"; }
    public function __CLONE() {}
    public function __INVOKE() {}
    public function __GET($name) { return null; }
    public function __SET($name, $value) {}
    public function __ISSET($name) { return false; }
    public function __UNSET($name) {}
    public function __CALL($name, $args) {}
    public static function __CALLSTATIC($name, $args) {}
    public function __debuginfo() { return []; }
}
echo "done\n";
?>
--EXPECTF--
Deprecated: Declaring Foo::__CONSTRUCT() with incorrect case is deprecated, use the correct casing __construct() instead in %s on line %d

Deprecated: Declaring Foo::__DESTRUCT() with incorrect case is deprecated, use the correct casing __destruct() instead in %s on line %d

Deprecated: Declaring Foo::__tostring() with incorrect case is deprecated, use the correct casing __toString() instead in %s on line %d

Deprecated: Declaring Foo::__CLONE() with incorrect case is deprecated, use the correct casing __clone() instead in %s on line %d

Deprecated: Declaring Foo::__INVOKE() with incorrect case is deprecated, use the correct casing __invoke() instead in %s on line %d

Deprecated: Declaring Foo::__GET() with incorrect case is deprecated, use the correct casing __get() instead in %s on line %d

Deprecated: Declaring Foo::__SET() with incorrect case is deprecated, use the correct casing __set() instead in %s on line %d

Deprecated: Declaring Foo::__ISSET() with incorrect case is deprecated, use the correct casing __isset() instead in %s on line %d

Deprecated: Declaring Foo::__UNSET() with incorrect case is deprecated, use the correct casing __unset() instead in %s on line %d

Deprecated: Declaring Foo::__CALL() with incorrect case is deprecated, use the correct casing __call() instead in %s on line %d

Deprecated: Declaring Foo::__CALLSTATIC() with incorrect case is deprecated, use the correct casing __callStatic() instead in %s on line %d

Deprecated: Declaring Foo::__debuginfo() with incorrect case is deprecated, use the correct casing __debugInfo() instead in %s on line %d
done
