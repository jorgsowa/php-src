--TEST--
FileSystemIterator without SKIP_DOTS
--FILE--
<?php

$dir = __DIR__ . '/filesystemiterator_no_skip_dots';
mkdir($dir);
touch($dir . '/file');

$it = new FileSystemIterator($dir, 0);
$files = [];
foreach ($it as $f) {
    $files[] = $f->getFileName();
}
sort($files);
var_dump($files);

?>
--CLEAN--
<?php
$dir = __DIR__ . '/filesystemiterator_no_skip_dots';
unlink($dir . '/file');
rmdir($dir);
?>
--EXPECTF--
Deprecated: Using FileSystemIterator as a class name with incorrect case is deprecated, use the correct casing FilesystemIterator instead in %s on line %d

Deprecated: Calling getFileName() is deprecated, use the correct casing SplFileInfo::getFilename() instead in %s on line %d
array(3) {
  [0]=>
  string(1) "."
  [1]=>
  string(2) ".."
  [2]=>
  string(4) "file"
}
