--TEST--
DCE must not remove FETCH_DIM_IS with a leading-numeric string offset
--INI--
opcache.enable=1
opcache.enable_cli=1
opcache.optimization_level=-1
opcache.file_update_protection=0
--EXTENSIONS--
opcache
--FILE--
<?php
set_error_handler(function ($errno, $errstr) {
    throw new Exception($errstr);
});

function test(string $s) {
    $x = $s["1x"] ?? null;
}

try {
    test("abc");
} catch (Exception $e) {
    echo $e::class, ": ", $e->getMessage(), "\n";
}
?>
--EXPECT--
Exception: Illegal string offset "1x"
