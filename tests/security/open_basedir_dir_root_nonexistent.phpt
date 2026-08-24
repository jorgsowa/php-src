--TEST--
Regression test for open_basedir on nonexistent absolute directory paths
--SKIPIF--
<?php
if (DIRECTORY_SEPARATOR === '\\') {
    die('skip Unix-specific root path test');
}
?>
--INI--
open_basedir=.
--FILE--
<?php
chdir(__DIR__);

$path = '/php-open-basedir-regression/nonexistent';
var_dump(dir($path));
?>
--EXPECTF--

Warning: dir(): open_basedir restriction in effect. File(/php-open-basedir-regression/nonexistent) is not within the allowed path(s): (.) in %s on line %d

Warning: dir(/php-open-basedir-regression/nonexistent): Failed to open directory: %s in %s on line %d
bool(false)
