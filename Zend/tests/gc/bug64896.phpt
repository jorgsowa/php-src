--TEST--
Bug #64896 (Segfault with gc_collect_cycles using unserialize on certain objects)
--INI--
zend.enable_gc=1
--FILE--
<?php
$bar = NULL;
class bad
{
    private $_private = array();

    public function __construct()
    {
        $this->_private[] = 'php';
    }

    public function __destruct()
    {
        global $bar;
        $bar = $this;
    }
}

$foo = new stdclass;
$foo->foo = $foo;
$foo->bad = new bad;

gc_disable();

unserialize(serialize($foo));
gc_collect_cycles();
var_dump($bar);
gc_enable();
/*  will output:
object(bad)#%d (1) {
  ["_private":"bad":private]=>
  &UNKNOWN:0
}
*/
?>
--EXPECTF--

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
object(bad)#%d (1) {
  ["_private":"bad":private]=>
  array(1) {
    [0]=>
    string(3) "php"
  }
}
