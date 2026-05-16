--TEST--
exception handler tests - 6
--FILE--
<?php

set_exception_handler("foo");
set_exception_handler("foo1");

restore_exception_handler();

function foo($e) {
    var_dump(__FUNCTION__."(): ".get_class($e)." thrown!");
}

function foo1($e) {
    var_dump(__FUNCTION__."(): ".get_class($e)." thrown!");
}


throw new excEption();

echo "Done\n";
?>
--EXPECTF--

Deprecated: Using excEption as a class name with incorrect case is deprecated, use the correct casing Exception instead in %s on line %d
string(24) "foo(): Exception thrown!"
