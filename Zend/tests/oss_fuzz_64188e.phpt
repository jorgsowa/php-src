--TEST--
oss-fuzz #64188: Fix unsupported operand types: null / null
--FILE--
<?php
set_error_handler(function () {
    unset($GLOBALS[""]);
});

$$ˇ /= $ˇ;

?>
--EXPECTF--
Fatal error: Uncaught TypeError: Unsupported operand types: null / null in %soss_fuzz_64188e.php:6
Stack trace:
#0 {main}
  thrown in %soss_fuzz_64188e.php on line 6
