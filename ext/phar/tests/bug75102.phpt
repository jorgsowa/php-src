--TEST--
Bug #75102 (`PharData` says invalid checksum for valid tar)
--EXTENSIONS--
phar
--FILE--
<?php
$phar = new PharData(__DIR__ . '/bug75102.tar');
var_dump(file_get_contents($phar['test.txt']->getPathName()));
?>
--EXPECTF--
Deprecated: Calling getPathName() is deprecated, use the correct casing SplFileInfo::getPathname() instead in %s on line %d
string(9) "yada yada"
