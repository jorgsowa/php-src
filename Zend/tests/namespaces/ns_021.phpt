--TEST--
021: Name search priority (first look into namespace)
--FILE--
<?php
namespace test;

class Test {
    static function foo() {
        echo __CLASS__,"::",__FUNCTION__,"\n";
    }
}

function foo() {
    echo __FUNCTION__,"\n";
}

foo();
\test\foo();
\test\test::foo();
?>
--EXPECTF--
test\foo
test\foo

Deprecated: Using test\test as a class name with incorrect case is deprecated, use the correct casing test\Test instead in %s on line %d
test\Test::foo
