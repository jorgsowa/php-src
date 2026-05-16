--TEST--
Bug #53144 (Segfault in SplObjectStorage::removeAll)
--FILE--
<?php

$o1 = new StdClass;
$o2 = new StdClass;

$b = new SplObjectStorage();
$b[$o1] = "bar";
$b[$o2] = "baz";

var_dump(count($b));
$b->removeAll($b);
var_dump(count($b));

?>
--EXPECTF--
Deprecated: Using StdClass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d

Deprecated: Using StdClass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
int(2)
int(0)
