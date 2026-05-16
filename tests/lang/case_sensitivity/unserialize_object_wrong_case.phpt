--TEST--
unserialize() with wrong-case class name emits E_DEPRECATED
--FILE--
<?php
class MyRow {
    public string $name = '';
}

// Correct case — no deprecation
$obj = unserialize('O:5:"MyRow":1:{s:4:"name";s:5:"hello";}');
echo get_class($obj) . "\n";

// Wrong case — E_DEPRECATED
$obj2 = unserialize('O:5:"MYROW":1:{s:4:"name";s:5:"hello";}');
echo get_class($obj2) . "\n";

$obj3 = unserialize('O:5:"myrow":1:{s:4:"name";s:5:"hello";}');
echo get_class($obj3) . "\n";
?>
--EXPECTF--
MyRow

Deprecated: Using MYROW as a class name with incorrect case is deprecated, use the correct casing MyRow instead in %s on line %d
MyRow

Deprecated: Using myrow as a class name with incorrect case is deprecated, use the correct casing MyRow instead in %s on line %d
MyRow
