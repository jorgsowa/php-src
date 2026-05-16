--TEST--
Function called with canonical (correct) case produces no deprecation
--FILE--
<?php
$result = strlen("hello");
echo $result . "\n";

function myUserFunc() { return 42; }
echo myUserFunc() . "\n";
?>
--EXPECT--
5
42
