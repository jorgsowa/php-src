--TEST--
Bug #46108 (DateTime - Memory leak when unserializing)
--FILE--
<?php

date_default_timezone_set('America/Sao_Paulo');

var_dump(unserialize(serialize(new Datetime)));

?>
--EXPECTF--
Deprecated: Using Datetime as a class name with incorrect case is deprecated, use the correct casing DateTime instead in %s on line %d
object(DateTime)#%d (3) {
  ["date"]=>
  string(%d) "%s"
  ["timezone_type"]=>
  int(%d)
  ["timezone"]=>
  string(%d) "America/Sao_Paulo"
}
