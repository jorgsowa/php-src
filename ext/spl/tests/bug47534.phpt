--TEST--
SPL: RecursiveDirectoryIterator bug 47534
--FILE--
<?php
$it1 = new RecursiveDirectoryIterator(__DIR__, FileSystemIterator::CURRENT_AS_PATHNAME);
$it1->rewind();
echo gettype($it1->current())."\n";

$it2 = new RecursiveDirectoryIterator(__DIR__);
$it2->rewind();
echo gettype($it2->current())."\n";
?>
--EXPECTF--
Deprecated: Using FileSystemIterator as a class name with incorrect case is deprecated, use the correct casing FilesystemIterator instead in %s on line %d
string
object
