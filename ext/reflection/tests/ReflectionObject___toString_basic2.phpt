--TEST--
ReflectionObject::__toString() : very basic test with dynamic properties
--FILE--
<?php

#[AllowDynamicProperties]
class Foo  {
    public $bar = 1;
}
$f = new foo;
$f->dynProp = 'hello';
$f->dynProp2 = 'hello again';
echo new ReflectionObject($f);

?>
--EXPECTF--
Deprecated: Using foo as a class name with incorrect case is deprecated, use the correct casing Foo instead in %s on line %d
Object of class [ <user> class Foo ] {
  @@ %s

  - Constants [0] {
  }

  - Static properties [0] {
  }

  - Static methods [0] {
  }

  - Properties [1] {
    Property [ public $bar = 1 ]
  }

  - Dynamic properties [2] {
    Property [ <dynamic> public $dynProp ]
    Property [ <dynamic> public $dynProp2 ]
  }

  - Methods [0] {
  }
}
