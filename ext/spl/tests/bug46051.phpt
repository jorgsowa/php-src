--TEST--
Bug #46051 (SplFileInfo::openFile - memory overlap)
--FILE--
<?php

$x = new splfileinfo(__FILE__);

try {
    $x->openFile("", false, []);
} catch (TypeError $e) { }

var_dump($x->getPathName());
?>
--EXPECTF--
Deprecated: Using splfileinfo as a class name with incorrect case is deprecated, use the correct casing SplFileInfo instead in %s on line %d

Deprecated: Calling getPathName() is deprecated, use the correct casing SplFileInfo::getPathname() instead in %s on line %d
string(%d) "%sbug46051.php"
