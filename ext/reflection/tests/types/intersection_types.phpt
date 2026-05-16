--TEST--
Intersection types in reflection
--FILE--
<?php

function dumpType(ReflectionIntersectionType $rt) {
    echo "Type $rt:\n";
    echo "Allows null: " . json_encode($rt->allowsNull()) . "\n";
    foreach ($rt->getTypes() as $type) {
        echo "  Name: " . $type->getName() . "\n";
        echo "  String: " . (string) $type . "\n";
        echo "  Allows Null: " . json_encode($type->allowsNull()) . "\n";
    }
}

function test1(): X&Y&Z&Traversable&Countable { }

class Test {
    public X&Y&Countable $prop;
}

dumpType((new ReflectionFunction('test1'))->getReturnType());

$rc = new ReflectionClass(Test::class);
$rp = $rc->getProperty('prop');
dumpType($rp->getType());

/* Force CE resolution of the property type */

interface y {}
class x implements Y, Countable {
    public function count(): int { return 0; }
}
$test = new Test;
$test->prop = new x;

$rp = $rc->getProperty('prop');
dumpType($rp->getType());

?>
--EXPECTF--
Type X&Y&Z&Traversable&Countable:
Allows null: false
  Name: X
  String: X
  Allows Null: false
  Name: Y
  String: Y
  Allows Null: false
  Name: Z
  String: Z
  Allows Null: false
  Name: Traversable
  String: Traversable
  Allows Null: false
  Name: Countable
  String: Countable
  Allows Null: false
Type X&Y&Countable:
Allows null: false
  Name: X
  String: X
  Allows Null: false
  Name: Y
  String: Y
  Allows Null: false
  Name: Countable
  String: Countable
  Allows Null: false

Deprecated: Using Y as a class name with incorrect case is deprecated, use the correct casing y instead in %s on line %d

Deprecated: Using X as a class name with incorrect case is deprecated, use the correct casing x instead in %s on line %d

Deprecated: Using Y as a class name with incorrect case is deprecated, use the correct casing y instead in %s on line %d
Type X&Y&Countable:
Allows null: false
  Name: X
  String: X
  Allows Null: false
  Name: Y
  String: Y
  Allows Null: false
  Name: Countable
  String: Countable
  Allows Null: false
