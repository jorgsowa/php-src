--TEST--
Testing type-hinted lambda parameter inside namespace
--FILE--
<?php

namespace foo;

$x = function (\stdclass $x = NULL) {
    var_dump($x);
};

$x(NULL);
$x(new \stdclass);

?>
--EXPECTF--

Deprecated: {closure:%s:%d}(): Implicitly marking parameter $x as nullable is deprecated, the explicit nullable type must be used instead in %s on line %d
NULL

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
object(stdClass)#%d (0) {
}
