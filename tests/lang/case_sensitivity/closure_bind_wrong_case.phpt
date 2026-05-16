--TEST--
Closure::bind() and bindTo() with wrong-case scope emits E_DEPRECATED
--FILE--
<?php
class MyScope {
    private int $x = 1;
}

$f = function() { return $this->x; };

$bound1 = Closure::bind($f, new MyScope, "myscope");
echo $bound1() . "\n";

$bound2 = $f->bindTo(new MyScope, "MYSCOPE");
echo $bound2() . "\n";
?>
--EXPECTF--
Deprecated: Using myscope as a class name with incorrect case is deprecated, use the correct casing MyScope instead in %s on line %d
1

Deprecated: Using MYSCOPE as a class name with incorrect case is deprecated, use the correct casing MyScope instead in %s on line %d
1
