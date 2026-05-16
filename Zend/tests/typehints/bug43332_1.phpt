--TEST--
Bug #43332.1 (self and parent as type hint in namespace)
--FILE--
<?php
namespace foobar;

class foo {
  public function bar(self $a) { }
}

$foo = new foo;
$foo->bar($foo); // Ok!
$foo->bar(new \stdclass); // Error, ok!
?>
--EXPECTF--

Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d

Fatal error: Uncaught TypeError: foobar\foo::bar(): Argument #1 ($a) must be of type foobar\foo, stdClass given, called in %s on line %d and defined in %s:%d
Stack trace:
#0 %s(%d): foobar\foo->bar(Object(stdClass))
#1 {main}
  thrown in %s on line %d
