--TEST--
Phar: test a zip archive created by unix "zip" command
--EXTENSIONS--
phar
--FILE--
<?php
$a = new PharData(__DIR__ . '/files/zip.zip');
foreach ($a as $b) {
    if ($b->isDir()) {
        echo "dir " . $b->getPathName() . "\n";
    } else {
        echo $b->getPathName(), "\n";
        echo file_get_contents($b->getPathName()), "\n";
    }
}
if (isset($a['notempty/hi.txt'])) {
    echo $a['notempty/hi.txt']->getPathName() . "\n";
}
?>
--EXPECTF--
Deprecated: Calling getPathName() is deprecated, use the correct casing SplFileInfo::getPathname() instead in %s on line %d
dir phar://%s/zip.zip%cempty

Deprecated: Calling getPathName() is deprecated, use the correct casing SplFileInfo::getPathname() instead in %s on line %d
phar://%s/zip.zip%chi.txt

Deprecated: Calling getPathName() is deprecated, use the correct casing SplFileInfo::getPathname() instead in %s on line %d
hi there

dir phar://%s/zip.zip%cnotempty

Deprecated: Calling getPathName() is deprecated, use the correct casing SplFileInfo::getPathname() instead in %s on line %d
phar://%s/zip.zip/notempty%chi.txt
