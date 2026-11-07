--TEST--
JIT FETCH_DIM_IS: leading-numeric string offset warning keeps the whole offset
--INI--
opcache.enable=1
opcache.enable_cli=1
opcache.jit=function
opcache.file_update_protection=0
--EXTENSIONS--
opcache
--FILE--
<?php
set_error_handler(function ($errno, $errstr) {
    echo json_encode($errstr), "\n";
    return true;
});

function test(string $s, string $k) {
    return $s[$k] ?? "default";
}

var_dump(test("abc", "1x\0y"));
?>
--EXPECT--
"Illegal string offset \"1x\u0000y\""
string(1) "b"
