--TEST--
Bug #76427 (Segfault in zend_objects_store_put)
--FILE--
<?php
$func = function () {
    yield 2;
};

$a  = new stdclass();
$b =  new stdclass();
$a->b = $b;
$b->a = $a;

$func = $a->func = $func();

unset($b);
unset($a);
unset($func);

var_dump(gc_collect_cycles());

?>
--EXPECTF--

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
int(2)
