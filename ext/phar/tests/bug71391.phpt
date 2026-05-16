--TEST--
Phar: bug #71391: NULL Pointer Dereference in phar_tar_setupmetadata()
--EXTENSIONS--
phar
--FILE--
<?php
// duplicate since the tar will change
copy(__DIR__."/bug71391.tar", __DIR__."/bug71391.test.tar");
$p = new PharData(__DIR__."/bug71391.test.tar");
$p->delMetaData();
?>
DONE
--CLEAN--
<?php
unlink(__DIR__."/bug71391.test.tar");
?>
--EXPECTF--
Deprecated: Calling delMetaData() is deprecated, use the correct casing PharData::delMetadata() instead in %s on line %d
DONE
