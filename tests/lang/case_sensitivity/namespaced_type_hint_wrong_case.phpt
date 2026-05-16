--TEST--
Namespaced type declarations with wrong-cased namespace emit E_DEPRECATED
--FILE--
<?php
namespace MyApp;

class Foo {}

function test(\myapp\Foo $x): \myapp\Foo { return $x; }

$obj = new Foo();
$result = test($obj);
echo get_class($result) . "\n";
?>
--EXPECTF--
Deprecated: Using myapp\Foo as a class name with incorrect case is deprecated, use the correct casing MyApp\Foo instead in %s on line %d
MyApp\Foo
