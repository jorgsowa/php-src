--TEST--
Testing parameter type-hinted (array) with default value inside namespace
--FILE--
<?php

namespace foo;

class bar {
    public function __construct(?array $x = NULL) {
        var_dump($x);
    }
}

new bar(null);
new bar(new \stdclass);

?>
--EXPECTF--
NULL

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d

Fatal error: Uncaught TypeError: foo\bar::__construct(): Argument #1 ($x) must be of type ?array, stdClass given, called in %s on line %d and defined in %s:%d
Stack trace:
#0 %s(%d): foo\bar->__construct(Object(stdClass))
#1 {main}
  thrown in %s on line %d
