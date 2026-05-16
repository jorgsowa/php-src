--TEST--
Method called explicitly with wrong case emits E_DEPRECATED
--FILE--
<?php
class Magic {
    public function __call($name, $args) { return "called $name"; }
}
$m = new Magic();
// Direct explicit call via callable array
$result = call_user_func([$m, '__CALL'], 'foo', []);
echo $result . "\n";
?>
--EXPECTF--
Deprecated: Calling __CALL() is deprecated, use the correct casing Magic::__call() instead in %s on line %d
called foo
