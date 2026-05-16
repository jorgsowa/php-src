--TEST--
ReflectionObject::__toString() : very basic test with no dynamic properties
--FILE--
<?php

class Foo  {
    public $bar = 1;
}
$f = new foo;

echo new ReflectionObject($f);

?>
--EXPECTF--
Deprecated: Using foo as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
Object of class [ <user> class Foo ] {
  @@ %s 3-5

  - Constants [0] {
  }

  - Static properties [0] {
  }

  - Static methods [0] {
  }

  - Properties [1] {
    Property [ public $bar = 1 ]
  }

  - Dynamic properties [0] {
  }

  - Methods [0] {
  }
}
