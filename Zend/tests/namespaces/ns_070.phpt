--TEST--
Testing parameter type-hinted with default value inside namespace
--FILE--
<?php

namespace foo;

class bar {
    public function __construct(?\stdclass $x = NULL) {
        var_dump($x);
    }
}

new bar(new \stdclass);
new bar(null);

?>
--EXPECTF--

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
object(stdClass)#%d (0) {
}
NULL
