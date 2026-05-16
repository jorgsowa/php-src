--TEST--
First-class callable syntax with wrong-cased function/method names emits E_DEPRECATED
--FILE--
<?php
function myHelper(): int { return 1; }
class C {
    public static function doStatic(): int { return 2; }
    public function doInstance(): int { return 3; }
}
$o = new C();

// Correct casing — no warning
$a = myHelper(...);
$b = C::doStatic(...);
$c = $o->doInstance(...);
echo $a(), $b(), $c(), "\n";

// Wrong casing — E_DEPRECATED for each
$d = MYHELPER(...);
$e = C::DOSTATIC(...);
$f = $o->DOINSTANCE(...);
echo $d(), $e(), $f(), "\n";
?>
--EXPECTF--
Deprecated: Calling MYHELPER() is deprecated, use the correct casing myHelper() instead in %s on line 16
123

Deprecated: Calling DOSTATIC() is deprecated, use the correct casing C::doStatic() instead in %s on line 17

Deprecated: Calling DOINSTANCE() is deprecated, use the correct casing C::doInstance() instead in %s on line 18
123
