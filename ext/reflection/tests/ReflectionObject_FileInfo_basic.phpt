--TEST--
ReflectionObject::getFileName(), ReflectionObject::getStartLine(), ReflectionObject::getEndLine() - basic function
--FILE--
<?php
$rc = new ReflectionObject(new C);
var_dump($rc->getFileName());
var_dump($rc->getStartLine());
var_dump($rc->getEndLine());

$rc = new ReflectionObject(new stdclass);
var_dump($rc->getFileName());
var_dump($rc->getStartLine());
var_dump($rc->getEndLine());

Class C {

}
?>
--EXPECTF--
string(%d) "%sReflectionObject_FileInfo_basic.php"
int(12)
int(14)

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
bool(false)
bool(false)
bool(false)
