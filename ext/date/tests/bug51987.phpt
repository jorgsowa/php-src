--TEST--
Bug #51987 (Datetime fails to parse an ISO 8601 ordinal date (extended format))
--FILE--
<?php
date_default_timezone_set('Europe/London');
$d2 = new Datetime('1985-102');
var_dump($d2);
?>
--EXPECTF--
Deprecated: Using Datetime as a class name with incorrect case is deprecated, use the correct casing DateTime instead in %s on line %d
object(DateTime)#%d (%d) {
  ["date"]=>
  string(26) "1985-04-12 00:00:00.000000"
  ["timezone_type"]=>
  int(3)
  ["timezone"]=>
  string(13) "Europe/London"
}
