--TEST--
GH-14639 (Member access within null pointer in ext/spl/spl_observer.c)
--INI--
memory_limit=2M
--SKIPIF--
<?php
if (getenv("USE_ZEND_ALLOC") === "0") {
    die("skip Zend MM disabled");
}
?>
--FILE--
<?php
$b = new SplObjectStorage();
for ($i = 10000; $i > 0; $i--) {
    $object = new StdClass();
    $object->a = str_repeat("a", 2);
    $b->offsetSet($object);
}
?>
--EXPECTF--
Deprecated: Using StdClass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d

Fatal error: Allowed memory size of %d bytes exhausted%s(tried to allocate %d bytes) in %s on line %d
