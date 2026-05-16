--TEST--
Bug #75607 (Comparison of initial static properties failing)
--FILE--
<?php

trait T1
{
    public static $prop1 = 1;
}

class Base
{
    public static $prop1 = 1;
}

class Child extends base
{
    use T1;
}

echo "DONE";

?>
--EXPECTF--
Deprecated: Using base as a class name with incorrect case is deprecated, use the correct casing Base instead in %s on line %d
DONE
