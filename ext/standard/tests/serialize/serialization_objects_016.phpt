--TEST--
Object serialization / unserialization: circular object with rc=1
--FILE--
<?php
$t=new stdClass;
$t->y=$t;
$y=(array)$t;
unset($t);
var_dump($y);
$s=serialize($y);
var_dump($s);
$x=unserialize($s);
var_dump($x);
vaR_dump(serialize($x));
?>
--EXPECTF--
Deprecated: Calling vaR_dump() is deprecated, use the correct casing var_dump() instead in %s on line %d
array(1) {
  ["y"]=>
  object(stdClass)#%d (1) {
    ["y"]=>
    *RECURSION*
  }
}
string(45) "a:1:{s:1:"y";O:8:"stdClass":1:{s:1:"y";r:2;}}"
array(1) {
  ["y"]=>
  object(stdClass)#%d (1) {
    ["y"]=>
    *RECURSION*
  }
}
string(45) "a:1:{s:1:"y";O:8:"stdClass":1:{s:1:"y";r:2;}}"
