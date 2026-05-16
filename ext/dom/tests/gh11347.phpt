--TEST--
GH-11347 (Memory leak when calling a static method inside an xpath query)
--EXTENSIONS--
dom
--FILE--
<?php

class MyClass
{
    public static function dump(string $var) {
        var_dump($var);
    }
}

$doc = new DOMDocument();
$doc->loadHTML('<a href="https://php.net">hello</a>');
$xpath = new DOMXpath($doc);
$xpath->registerNamespace("php", "http://php.net/xpath");
$xpath->registerPHPFunctions();
$xpath->query("//a[php:function('MyClass::dump', string(@href))]");

?>
Done
--EXPECTF--
Deprecated: Using DOMXpath as a class name with incorrect case is deprecated, use the correct casing DOMXPath instead in %s on line %d

Deprecated: Calling registerPHPFunctions() is deprecated, use the correct casing DOMXPath::registerPhpFunctions() instead in %s on line %d
string(15) "https://php.net"
Done
