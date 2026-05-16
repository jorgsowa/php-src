--TEST--
Phar object: iterate test with sub-directories and RecursiveIteratorIterator
--EXTENSIONS--
phar
--INI--
phar.readonly=0
phar.require_hash=0
--FILE--
<?php
$fname = __DIR__ . '/' . basename(__FILE__, '.php') . '.phar.php';

$phar = new Phar($fname);
$phar['top.txt'] = 'hi';
$phar['sub/top.txt'] = 'there';
$phar['another.file.txt'] = 'wowee';
$newphar = new Phar($fname);
foreach (new RecursiveIteratorIterator($newphar) as $path => $obj) {
    var_dump($obj->getPathName());
}
?>
--CLEAN--
<?php
unlink(__DIR__ . '/' . basename(__FILE__, '.clean.php') . '.phar.php');
__halt_compiler();
?>
--EXPECTF--
Deprecated: Calling getPathName() is deprecated, use the correct casing SplFileInfo::getPathname() instead in %s on line %d
string(%d) "phar://%sphar_dir_iterate.phar.php%canother.file.txt"
string(%d) "phar://%sphar_dir_iterate.phar.php/sub%ctop.txt"
string(%d) "phar://%sphar_dir_iterate.phar.php%ctop.txt"
