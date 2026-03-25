--TEST--
JIT ROPE: 003 IS_CONST string in rope must not be freed on repeated JIT trace execution
--DESCRIPTION--
Regression test for GH-21419: heap-use-after-free in zend_jit_rope_end with --repeat.
When a function containing string interpolation with IS_CONST strings is JIT-compiled,
the JIT must addref non-interned constant strings before storing them in the rope buffer,
matching the interpreter's behavior. Without the fix, the constant string pointer is
stored without an addref, causing zend_jit_rope_end to free it, and the next trace
execution reads the freed memory.
--INI--
opcache.enable=1
opcache.enable_cli=1
opcache.file_update_protection=0
opcache.jit=tracing
opcache.jit_hot_func=1
--FILE--
<?php
// Use eval so the op_array is not in SHM; its IS_CONST strings are NOT interned,
// making them refcounted. Without the fix the JIT trace would UAF on the 2nd call.
$fn = eval('return function($x) { return "hello $x world"; };');

// Call enough times to trigger JIT compilation and then exercise the compiled trace.
for ($i = 0; $i < 5; $i++) {
    $result = $fn("PHP");
    if ($result !== "hello PHP world") {
        echo "FAIL at iteration $i: got $result\n";
        exit(1);
    }
}
echo "PASS\n";
?>
--EXPECT--
PASS
