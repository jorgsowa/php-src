--TEST--
Only arrays and Traversables can be unpacked
--FILE--
<?php

function test(...$args) {
    var_dump($args);
}

try {
    test(...null);
} catch (Error $e) {
    echo $e::class . ": " . $e->getMessage(), "\n";
}
try {
    test(...42);
} catch (Error $e) {
    echo $e::class . ": " . $e->getMessage(), "\n";
}
try {
    test(...new stdClass);
} catch (Error $e) {
    echo $e::class . ": " . $e->getMessage(), "\n";
}

try {
    test(1, 2, 3, ..."foo", ...[4, 5]);
} catch (Error $e) {
    echo $e::class . ": " . $e->getMessage(), "\n";
}
try {
    test(1, 2, 3, ...new StdClass, ...3.14, ...[4, 5]);
} catch (Error $e) {
    echo $e::class . ": " . $e->getMessage(), "\n";
}

?>
--EXPECTF--
TypeError: Only arrays and Traversables can be unpacked, null given
TypeError: Only arrays and Traversables can be unpacked, int given
TypeError: Only arrays and Traversables can be unpacked, stdClass given
TypeError: Only arrays and Traversables can be unpacked, string given

Deprecated: Using StdClass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
TypeError: Only arrays and Traversables can be unpacked, stdClass given
