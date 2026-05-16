--TEST--
Exception thrown by user error handler during catch-clause case deprecation propagates
--FILE--
<?php
set_error_handler(function (int $errno, string $msg): bool {
    echo "handler: $msg\n";
    throw new RuntimeException("thrown from handler");
});

class FooException extends Exception {}

try {
    try {
        throw new FooException('original');
    } catch (FOOEXCEPTION $e) {
        echo "should not reach inner catch body\n";
    }
} catch (Throwable $t) {
    echo "outer: " . get_class($t) . ": " . $t->getMessage() . "\n";
}
echo "done\n";
?>
--EXPECTF--
handler: Using FOOEXCEPTION as a class name with incorrect case is deprecated, use the correct casing FooException instead
outer: RuntimeException: thrown from handler
done
