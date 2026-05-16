--TEST--
Testing type-hinted lambda parameter inside namespace
--FILE--
<?php

namespace foo;

$x = function (?\stdclass $x = NULL) {
    var_dump($x);
};

class stdclass extends \stdclass { }

$x(NULL);
$x(new stdclass);
$x(new \stdclass);

?>
--EXPECTF--
NULL
object(foo\stdclass)#%d (0) {
}

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
object(stdClass)#%d (0) {
}
