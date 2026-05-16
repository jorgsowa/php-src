--TEST--
Indirect method call and cloning
--FILE--
<?php


class bar {
    public $z;

    public function __construct() {
        $this->z = new stdclass;
    }
    public function getZ() {
        return $this->z;
    }
}

var_dump(clone (new bar)->z);
var_dump(clone (new bar)->getZ());

?>
--EXPECTF--

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
object(stdClass)#%d (0) {
}
object(stdClass)#%d (0) {
}
