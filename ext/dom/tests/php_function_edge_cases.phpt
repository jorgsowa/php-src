--TEST--
php:function() edge cases
--EXTENSIONS--
dom
--FILE--
<?php

$doc = new DOMDocument();
$doc->loadHTML('<a href="https://php.net">hello</a>');
$xpath = new DOMXpath($doc);
$xpath->registerNamespace("php", "http://php.net/xpath");
$xpath->registerPHPFunctions();
try {
    $xpath->query("//a[php:function(3)]");
} catch (TypeError $e) {
    echo $e->getMessage(), "\n";
}
try {
    $xpath->query("//a[php:function()]");
} catch (Throwable $e) {
    echo $e->getMessage(), "\n";
}

?>
--EXPECTF--
Deprecated: Using DOMXpath as a class name with incorrect case is deprecated, use the correct casing DOMXPath instead in %s on line %d

Deprecated: Calling registerPHPFunctions() is deprecated, use the correct casing DOMXPath::registerPhpFunctions() instead in %s on line %d
Handler name must be a string
Function name must be passed as the first argument
