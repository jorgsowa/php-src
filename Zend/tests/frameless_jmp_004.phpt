--TEST--
Frameless jmp
--FILE--
<?php
namespace Foo;
function declare_local_class_exists() {
    function class_exists() {
        var_dump(__FUNCTION__);
        return true;
    }
}
declare_local_class_exists();
var_dump(CLASS_EXISTS('Foo'));
?>
--EXPECTF--
Deprecated: Calling Foo\CLASS_EXISTS() is deprecated, use the correct casing Foo\class_exists() instead in %s on line %d
string(16) "Foo\class_exists"
bool(true)
