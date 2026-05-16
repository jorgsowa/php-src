--TEST--
method_exists() on internal classes
--FILE--
<?php
echo " ---(Internal classes, using string class name)---\n";
echo "Does exception::getmessage exist? ";
var_dump(method_exists("exception", "getmessage"));
echo "Does stdclass::nonexistent exist? ";
var_dump(method_exists("stdclass", "nonexistent"));

echo "\n ---(Internal classes, using class instance)---\n";
echo "Does exception::getmessage exist? ";
var_dump(method_exists(new exception, "getmessage"));
echo "Does stdclass::nonexistent exist? ";
var_dump(method_exists(new stdclass, "nonexistent"));

echo "Done";
?>
--EXPECTF--
 ---(Internal classes, using string class name)---
Does exception::getmessage exist? 
Deprecated: Using exception as a class name with incorrect case is deprecated, use the correct casing Exception instead in %s on line %d

Deprecated: Calling getmessage() is deprecated, use the correct casing Exception::getMessage() instead in %s on line %d
bool(true)
Does stdclass::nonexistent exist? 
Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
bool(false)

 ---(Internal classes, using class instance)---
Does exception::getmessage exist? 
Deprecated: Using exception as a class name with incorrect case is deprecated, use the correct casing Exception instead in %s on line %d

Deprecated: Calling getmessage() is deprecated, use the correct casing Exception::getMessage() instead in %s on line %d
bool(true)
Does stdclass::nonexistent exist? 
Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
bool(false)
Done
