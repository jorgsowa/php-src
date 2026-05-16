--TEST--
Bug #70332 (Wrong behavior while returning reference on object)
--FILE--
<?php
function & test($arg) {
    return $arg;
}

$arg = new Stdclass();
$arg->name = array();

test($arg)->name[1] = "xxxx";

print_r($arg);
?>
--EXPECTF--

Deprecated: Using Stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
stdClass Object
(
    [name] => Array
        (
            [1] => xxxx
        )

)
