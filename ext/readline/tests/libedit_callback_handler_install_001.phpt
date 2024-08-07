--TEST--
readline_callback_handler_install(): Basic test
lsan disabled due to a leak on ubuntu focal only.
--EXTENSIONS--
readline
--SKIPIF--
<?php if (!function_exists('readline_callback_handler_install')) die("skip");
if (READLINE_LIB != "libedit") die("skip libedit only");
?>
--INI--
zend.signal_check=0
--ENV--
LSAN_OPTIONS=detect_leaks=0
--FILE--
<?php

function foo() {
    readline_callback_handler_remove();
}

var_dump(readline_callback_handler_install('testing: ', 'foo'));
try {
    var_dump(readline_callback_handler_install('testing: ', 'foobar!'));
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}

?>
--EXPECT--
bool(true)
readline_callback_handler_install(): Argument #2 ($callback) must be a valid callback, function "foobar!" not found or invalid function name
