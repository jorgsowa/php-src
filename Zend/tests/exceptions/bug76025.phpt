--TEST--
Bug #76025 (Segfault while throwing exception in error_handler)
--FILE--
<?php

function handleError($errno, $errstr, $errfile, $errline) {
    $exception = new exception("blah");
    throw $exception;
}
set_error_handler('handleError', E_ALL);
$c = $b[$a];
?>
--EXPECTF--

Deprecated: Using exception as a class name with incorrect case is deprecated, use the correct casing Exception instead in %s on line %d

Fatal error: Uncaught Exception: blah in %s:%d
Stack trace:
#0 %s(%d): handleError(2, 'Undefined varia...', '%s', %d)
#1 {main}
  thrown in %s on line %d
