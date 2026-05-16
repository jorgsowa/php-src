--TEST--
Bug #51827 (Bad warning when register_shutdown_function called with wrong num of parameters)
--FILE--
<?php


function abc() {
    var_dump(1);
}

register_shutdown_function('timE');
register_shutdown_function('ABC');
register_shutdown_function('exploDe');

?>
--EXPECTF--
Deprecated: Calling timE() is deprecated, use the correct casing time() instead in %s on line %d

Deprecated: Calling ABC() is deprecated, use the correct casing abc() instead in %s on line %d

Deprecated: Calling exploDe() is deprecated, use the correct casing explode() instead in %s on line %d
int(1)

Fatal error: Uncaught ArgumentCountError: explode() expects at least 2 arguments, 0 given in [no active file]:0
Stack trace:
#0 [internal function]: explode()
#1 {main}
  thrown in [no active file] on line 0
