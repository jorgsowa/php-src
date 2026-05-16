--TEST--
Test property_exists() function : class auto loading
--FILE--
<?php
echo "*** Testing property_exists() : class auto loading ***\n";

spl_autoload_register(function ($class_name) {
    require_once $class_name . '.inc';
});

echo "\ntesting property in autoloaded class\n";
var_dump(property_exists("AutoTest", "bob"));

echo "\ntesting __get magic method\n";
var_dump(property_exists("AutoTest", "foo"));

?>
--EXPECTF--
*** Testing property_exists() : class auto loading ***

testing property in autoloaded class

Deprecated: Using AutoTest as a class name with incorrect case is deprecated, use the correct casing autoTest instead in %s on line %d
bool(true)

testing __get magic method

Deprecated: Using AutoTest as a class name with incorrect case is deprecated, use the correct casing autoTest instead in %s on line %d
bool(false)
