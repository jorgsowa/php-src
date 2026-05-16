--TEST--
Class name with incorrect case in catch clause reaches user error handler
--FILE--
<?php
class FooException extends Exception {}

set_error_handler(function (int $errno, string $msg): bool {
    echo "handler: $msg\n";
    return true;
});

try {
    throw new FooException('test');
} catch (FOOEXCEPTION $e) {
    echo "caught\n";
}
?>
--EXPECTF--
handler: Using FOOEXCEPTION as a class name with incorrect case is deprecated, use the correct casing FooException instead
caught
