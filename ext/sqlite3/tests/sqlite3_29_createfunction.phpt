--TEST--
SQLite3::createFunction - Basic test
--EXTENSIONS--
sqlite3
--FILE--
<?php

require_once(__DIR__ . '/new_db.inc');

$func = 'strtoupper';
var_dump($db->createfunction($func, $func));
var_dump($db->querySingle("SELECT strtoupper('test')"));

$func2 = 'strtolower';
var_dump($db->createfunction($func2, $func2));
var_dump($db->querySingle("SELECT strtolower('TEST')"));

var_dump($db->createfunction($func, $func2));
var_dump($db->querySingle("SELECT strtoupper('tEst')"));


?>
--EXPECTF--
Deprecated: Calling createfunction() is deprecated, use the correct casing SQLite3::createFunction() instead in %s on line %d
bool(true)
string(4) "TEST"

Deprecated: Calling createfunction() is deprecated, use the correct casing SQLite3::createFunction() instead in %s on line %d
bool(true)
string(4) "test"

Deprecated: Calling createfunction() is deprecated, use the correct casing SQLite3::createFunction() instead in %s on line %d
bool(true)
string(4) "test"
