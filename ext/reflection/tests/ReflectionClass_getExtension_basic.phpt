--TEST--
ReflectionClass::getExtension() method - basic test for getExtension() method
--EXTENSIONS--
dom
--CREDITS--
Rein Velt <rein@velt.org>
#testFest Roosendaal 2008-05-10
--FILE--
<?php
    $rc=new reflectionClass('domDocument');
    var_dump($rc->getExtension()) ;
?>
--EXPECTF--
Deprecated: Using reflectionClass as a class name with incorrect case is deprecated, use the correct casing ReflectionClass instead in %s on line %d

Deprecated: Using domDocument as a class name with incorrect case is deprecated, use the correct casing DOMDocument instead in %s on line %d
object(ReflectionExtension)#%d (1) {
  ["name"]=>
  string(3) "dom"
}
