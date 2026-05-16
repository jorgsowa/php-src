--TEST--
Array and string callables with wrong-case class name emit E_DEPRECATED
--FILE--
<?php
class MyService {
    public static function process(): string { return "ok"; }
}

// Array callable — wrong-cased class name
$result = call_user_func(["MYSERVICE", "process"]);
echo $result . "\n";

$result2 = call_user_func(["myservice", "process"]);
echo $result2 . "\n";

// String callable — wrong-cased class name
$result3 = call_user_func("MYSERVICE::process");
echo $result3 . "\n";

// Correct — no warning
$result4 = call_user_func(["MyService", "process"]);
echo $result4 . "\n";
?>
--EXPECTF--
Deprecated: Using MYSERVICE as a class name with incorrect case is deprecated, use the correct casing MyService instead in %s on line %d
ok

Deprecated: Using myservice as a class name with incorrect case is deprecated, use the correct casing MyService instead in %s on line %d
ok

Deprecated: Using MYSERVICE as a class name with incorrect case is deprecated, use the correct casing MyService instead in %s on line %d
ok
ok
