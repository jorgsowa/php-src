--TEST--
"Reference Unpacking - New Reference" list()
--FILE--
<?php
$arr = array(new stdclass);
list(&$a, &$b) = $arr;
var_dump($a, $b);
var_dump($arr);
?>
--EXPECTF--

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
object(stdClass)#%d (0) {
}
NULL
array(2) {
  [0]=>
  &object(stdClass)#%d (0) {
  }
  [1]=>
  &NULL
}
