--TEST--
SPL: SplObjectStorage: recursive var_dump
--FILE--
<?php
$o = new SplObjectStorage();

$o[new StdClass] = $o;

var_dump($o);
?>
--EXPECTF--
Deprecated: Using StdClass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
object(SplObjectStorage)#%d (1) {
  ["storage":"SplObjectStorage":private]=>
  array(1) {
    [0]=>
    array(2) {
      ["obj"]=>
      object(stdClass)#%d (0) {
      }
      ["inf"]=>
      *RECURSION*
    }
  }
}
