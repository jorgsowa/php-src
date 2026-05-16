--TEST--
Phar: verify stub of specific length does not break __HALT_COMPILER(); scanning in php
--EXTENSIONS--
phar
--INI--
phar.require_hash=0
phar.readonly=0
--FILE--
<?php
$phar = __DIR__ . '/files/stuboflength1041.phar';
foreach (new RecursiveIteratorIterator(new Phar($phar, alias: 'stuboflength1041.phar')) as $item) {
    var_dump($item->getFileName());
}
?>
--EXPECTF--
Deprecated: Calling getFileName() is deprecated, use the correct casing SplFileInfo::getFilename() instead in %s on line %d
string(5) "a.php"
string(5) "b.php"
