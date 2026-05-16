--TEST--
Testing PDORow and PDOStatement instances with Reflection
--EXTENSIONS--
pdo
--FILE--
<?php

$instance = new reflectionclass('pdostatement');
$x = $instance->newInstance();
var_dump($x);

// Allow initializing assignment.
$x->queryString = "SELECT 1";
var_dump($x);
// But don't allow reassignment.
try {
    $x->queryString = "SELECT 2";
    var_dump($x);
} catch (Error $e) {
    echo $e->getMessage(), "\n";
}

$instance = new reflectionclass('pdorow');
$x = $instance->newInstance();
var_dump($x);

?>
--EXPECTF--
Deprecated: Using reflectionclass as a class name with incorrect case is deprecated, use the correct casing ReflectionClass instead in %s on line %d

Deprecated: Using pdostatement as a class name with incorrect case is deprecated, use the correct casing PDOStatement instead in %s on line %d
object(PDOStatement)#%d (0) {
  ["queryString"]=>
  uninitialized(string)
}
object(PDOStatement)#2 (1) {
  ["queryString"]=>
  string(8) "SELECT 1"
}
Property queryString is read only

Deprecated: Using reflectionclass as a class name with incorrect case is deprecated, use the correct casing ReflectionClass instead in %s on line %d

Deprecated: Using pdorow as a class name with incorrect case is deprecated, use the correct casing PDORow instead in %s on line %d

Fatal error: Uncaught PDOException: You may not create a PDORow manually in %spdo_036.php:%d
Stack trace:
#0 %spdo_036.php(%d): ReflectionClass->newInstance()
#1 {main}
  thrown in %spdo_036.php on line %d
