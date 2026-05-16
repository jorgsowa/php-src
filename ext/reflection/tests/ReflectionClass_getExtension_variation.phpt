--TEST--
ReflectionClass::getExtension() method - variation test for getExtension()
--CREDITS--
Rein Velt <rein@velt.org>
#testFest Roosendaal 2008-05-10
--FILE--
<?php

    class myClass
    {
        public $varX;
        public $varY;
    }
    $rc=new reflectionClass('myClass');
    var_dump( $rc->getExtension()) ;
?>
--EXPECTF--
Deprecated: Using reflectionClass as a class name with incorrect case is deprecated, use the correct casing ReflectionClass instead in %s on line %d
NULL
