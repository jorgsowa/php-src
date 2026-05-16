--TEST--
022: Name search priority (first look into import, then into current namespace and then for class)
--FILE--
<?php
namespace a\b\c;

use a\b\c as test;

require "ns_022.inc";

function foo() {
    echo __FUNCTION__,"\n";
}

test\foo();
\test::foo();
?>
--EXPECTF--
a\b\c\foo

Deprecated: Using test as a class name with incorrect case is deprecated, use the correct casing Test instead in %s on line %d
Test::foo
