--TEST--
Phar: bug #46032: PharData::__construct wrong memory read
--EXTENSIONS--
phar
--SKIPIF--
<?php if (getenv('SKIP_SLOW_TESTS')) die('skip'); ?>
--FILE--
<?php

$a = __DIR__ .'/mytest';

try {
    new phar($a);
} catch (exception $e) { }

var_dump($a);

try {
    new phar($a);
} catch (exception $e) { }

var_dump($a);

new phardata('0000000000000000000');
?>
===DONE===
--EXPECTF--
Deprecated: Using phar as a class name with incorrect case is deprecated, use the correct casing Phar instead in %s on line %d

Deprecated: Using exception as a class name with incorrect case is deprecated, use the correct casing Exception instead in %s on line %d
string(%d) "%smytest"

Deprecated: Using phar as a class name with incorrect case is deprecated, use the correct casing Phar instead in %s on line %d

Deprecated: Using exception as a class name with incorrect case is deprecated, use the correct casing Exception instead in %s on line %d
string(%d) "%smytest"

Deprecated: Using phardata as a class name with incorrect case is deprecated, use the correct casing PharData instead in %s on line %d

Fatal error: Uncaught UnexpectedValueException: Cannot create phar '0000000000000000000', file extension (or combination) not recognised or the directory does not exist in %sbug46032.php:%d
Stack trace:
#0 %sbug46032.php(%d): PharData->__construct('000000000000000...')
#1 {main}
  thrown in %sbug46032.php on line %d
