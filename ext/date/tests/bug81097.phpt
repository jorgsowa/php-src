--TEST--
Bug #81097 (DateTimeZone silently falls back to UTC when providing an offset with seconds)
--FILE--
<?php
$d = new DatetimeZone('+01:45:30');
var_dump($d);
?>
--EXPECTF--
Deprecated: Using DatetimeZone as a class name with incorrect case is deprecated, use the correct casing DateTimeZone instead in %s on line %d
object(DateTimeZone)#%d (%d) {
  ["timezone_type"]=>
  int(1)
  ["timezone"]=>
  string(9) "+01:45:30"
}
