--TEST--
Test class_exists() function : basic functionality
--FILE--
<?php
echo "*** Testing class_exists() : basic functionality ***\n";

spl_autoload_register(function ($className) {
    echo "In autoload($className)\n";
});

echo "Calling class_exists() on non-existent class with autoload explicitly enabled:\n";
var_dump( class_exists('C', true) );
echo "\nCalling class_exists() on existing class with autoload explicitly enabled:\n";
var_dump( class_exists('stdclass', true) );

echo "\nCalling class_exists() on non-existent class with autoload explicitly enabled:\n";
var_dump( class_exists('D', false) );
echo "\nCalling class_exists() on existing class with autoload explicitly disabled:\n";
var_dump( class_exists('stdclass', false) );

echo "\nCalling class_exists() on non-existent class with autoload unspecified:\n";
var_dump( class_exists('E') );
echo "\nCalling class_exists() on existing class with autoload unspecified:\n";
var_dump( class_exists('stdclass') );

echo "Done";
?>
--EXPECTF--
*** Testing class_exists() : basic functionality ***
Calling class_exists() on non-existent class with autoload explicitly enabled:
In autoload(C)
bool(false)

Calling class_exists() on existing class with autoload explicitly enabled:

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
bool(true)

Calling class_exists() on non-existent class with autoload explicitly enabled:
bool(false)

Calling class_exists() on existing class with autoload explicitly disabled:

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
bool(true)

Calling class_exists() on non-existent class with autoload unspecified:
In autoload(E)
bool(false)

Calling class_exists() on existing class with autoload unspecified:

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
bool(true)
Done
