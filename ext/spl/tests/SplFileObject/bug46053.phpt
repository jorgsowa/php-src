--TEST--
Bug #46053 (SplFileObject::seek - Endless loop)
--FILE--
<?php

$x = new splfileobject(__FILE__);
$x->getPathName();
$x->seek(10);
$x->seek(0);
var_dump(trim($x->fgets()));
?>
--EXPECTF--
Deprecated: Using splfileobject as a class name with incorrect case is deprecated, use the correct casing SplFileObject instead in %s on line %d

Deprecated: Calling getPathName() is deprecated, use the correct casing SplFileInfo::getPathname() instead in %s on line %d
string(%d) "<?php"
