--TEST--
SPL: SplObjectStorage addAll/removeAll
--FILE--
<?php
class A extends SplObjectStorage { }

$o1 = new StdClass;
$o2 = new StdClass;
$o3 = new StdClass;

$a = new A;
$a->offsetSet($o1);
$a->offsetSet($o2);

$b = new SplObjectStorage();
$b->offsetSet($o2);
$b->offsetSet($o3);

$a->offsetUnset($b);

var_dump($a->count());

$a->offsetUnset($o3);
var_dump($a->count());

$a->removeAll($b);
var_dump($a->count());
?>
--EXPECTF--
Deprecated: Using StdClass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d

Deprecated: Using StdClass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d

Deprecated: Using StdClass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
int(2)
int(2)
int(1)
