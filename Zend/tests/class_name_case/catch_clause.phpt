--TEST--
Class name with incorrect case is deprecated in catch clause
--FILE--
<?php
class FooException extends Exception {}

// correct case, no deprecation
try {
    throw new FooException('test');
} catch (FooException $e) {
    echo "caught\n";
}

// wrong case
try {
    throw new FooException('test');
} catch (FOOEXCEPTION $e) {
    echo "caught\n";
}

// wrong case, exception does not match this catch (still warns on cache miss)
try {
    try {
        throw new RuntimeException('test');
    } catch (FOOEXCEPTION $e) {
        echo "should not reach\n";
    }
} catch (RuntimeException $e) {
    echo "rethrown\n";
}
?>
--EXPECTF--
caught

Deprecated: Using FOOEXCEPTION as a class name with incorrect case is deprecated, use the correct casing FooException instead in %s on line %d
caught

Deprecated: Using FOOEXCEPTION as a class name with incorrect case is deprecated, use the correct casing FooException instead in %s on line %d
rethrown
