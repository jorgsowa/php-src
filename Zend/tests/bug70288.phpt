--TEST--
Bug #70288 (Apache crash related to ZEND_SEND_REF)
--FILE--
<?php
class A {
    public function __get($name) {
        return new Stdclass();
    }
}

function test(&$obj) {
    var_dump($obj);
}
$a = new A;
test($a->dummy);
?>
--EXPECTF--

Deprecated: Using Stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
object(stdClass)#%d (0) {
}
