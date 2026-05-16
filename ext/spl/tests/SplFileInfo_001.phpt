--TEST--
Testing SplFileInfo calling the constructor twice
--FILE--
<?php
$x = new splfileinfo(1);
$x->__construct(1);

echo "done!\n";
?>
--EXPECTF--
Deprecated: Using splfileinfo as a class name with incorrect case is deprecated, use the correct casing SplFileInfo instead in %s on line %d
done!
