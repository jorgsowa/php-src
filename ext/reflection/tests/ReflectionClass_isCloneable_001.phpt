--TEST--
Testing ReflectionClass::isCloneable()
--EXTENSIONS--
simplexml
xmlwriter
--FILE--
<?php

class foo {
}
$foo = new foo;

print "User class\n";
$obj = new ReflectionClass($foo);
var_dump($obj->isCloneable());
$obj = new ReflectionObject($foo);
var_dump($obj->isCloneable());
$h = clone $foo;

class bar {
    private function __clone() {
    }
}
$bar = new bar;
print "User class - private __clone\n";
$obj = new ReflectionClass($bar);
var_dump($obj->isCloneable());
$obj = new ReflectionObject($bar);
var_dump($obj->isCloneable());
$h = clone $foo;

print "Closure\n";
$closure = function () { };
$obj = new ReflectionClass($closure);
var_dump($obj->isCloneable());
$obj = new ReflectionObject($closure);
var_dump($obj->isCloneable());
$h = clone $closure;

print "Internal class - SimpleXMLElement\n";
$obj = new ReflectionClass('simplexmlelement');
var_dump($obj->isCloneable());
$obj = new ReflectionObject(new simplexmlelement('<test></test>'));
var_dump($obj->isCloneable());
$h = clone new simplexmlelement('<test></test>');

print "Internal class - XMLWriter\n";
$obj = new ReflectionClass('xmlwriter');
var_dump($obj->isCloneable());
$obj = new ReflectionObject(new XMLWriter);
var_dump($obj->isCloneable());
try {
    $h = clone new xmlwriter;
} catch (Throwable $e) {
    echo $e::class, ": ", $e->getMessage(), PHP_EOL;
}

?>
--EXPECTF--
User class
bool(true)
bool(true)
User class - private __clone
bool(false)
bool(false)
Closure
bool(true)
bool(true)
Internal class - SimpleXMLElement

Deprecated: Using simplexmlelement as a class name with incorrect case is deprecated, use the correct casing SimpleXMLElement instead in %s on line %d
bool(true)

Deprecated: Using simplexmlelement as a class name with incorrect case is deprecated, use the correct casing SimpleXMLElement instead in %s on line %d
bool(true)

Deprecated: Using simplexmlelement as a class name with incorrect case is deprecated, use the correct casing SimpleXMLElement instead in %s on line %d
Internal class - XMLWriter

Deprecated: Using xmlwriter as a class name with incorrect case is deprecated, use the correct casing XMLWriter instead in %s on line %d
bool(false)
bool(false)

Deprecated: Using xmlwriter as a class name with incorrect case is deprecated, use the correct casing XMLWriter instead in %s on line %d
Error: Trying to clone an uncloneable object of class XMLWriter
