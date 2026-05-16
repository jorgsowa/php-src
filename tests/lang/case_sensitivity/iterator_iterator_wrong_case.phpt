--TEST--
IteratorIterator and RecursiveIteratorIterator class cast with wrong case emits E_DEPRECATED
--FILE--
<?php
class MyAggregate implements IteratorAggregate {
    public function getIterator(): ArrayIterator {
        return new ArrayIterator([1, 2, 3]);
    }
}

$it = new IteratorIterator(new MyAggregate(), "MYAGGREGATE");
echo "ok\n";
?>
--EXPECTF--
Deprecated: Using MYAGGREGATE as a class name with incorrect case is deprecated, use the correct casing MyAggregate instead in %s on line %d
ok
