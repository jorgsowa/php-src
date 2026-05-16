--TEST--
class_alias() with wrong-case class name emits E_DEPRECATED
--FILE--
<?php
class MyClass {}
class_alias("myclass", "Alias1");
class_alias("MYCLASS", "Alias2");
echo "done\n";
?>
--EXPECTF--
Deprecated: Using myclass as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d

Deprecated: Using MYCLASS as a class name with incorrect case is deprecated, use the correct casing MyClass instead in %s on line %d
done
