--TEST--
Testing array dereference on __invoke() result
--FILE--
<?php

error_reporting(E_ALL);

class foo {
    public $x = array();
    public function __construct() {
        $h = array();
        $h[] = new stdclass;
        $this->x = $h;
    }
    public function __invoke() {
        return $this->x;
    }
}


$fo = new foo;
var_dump($fo()[0]);

?>
--EXPECTF--

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
object(stdClass)#%d (0) {
}
