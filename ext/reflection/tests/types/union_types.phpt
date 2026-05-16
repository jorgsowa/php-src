--TEST--
Union types in reflection
--FILE--
<?php

function dumpType(ReflectionUnionType $rt) {
    echo "Type $rt:\n";
    echo "Allows null: " . ($rt->allowsNull() ? "true" : "false") . "\n";
    foreach ($rt->getTypes() as $type) {
        echo "  Name: " . $type->getName() . "\n";
        echo "  String: " . (string) $type . "\n";
        echo "  Allows Null: " . ($type->allowsNull() ? "true" : "false") . "\n";
    }
}

function dumpBCType(ReflectionNamedType $rt) {
    echo "Type $rt:\n";
    echo "  Name: " . $rt->getName() . "\n";
    echo "  String: " . (string) $rt . "\n";
    echo "  Allows Null: " . ($rt->allowsNull() ? "true" : "false") . "\n";
}

function test1(): X|Y|int|float|false|null { }
function test2(): X|iterable|bool { }
function test3(): null|false { }
function test4(): ?false { }
function test5(): X|iterable|true { }
function test6(): null|true { }
function test7(): ?true { }

class Test {
    public X|Y|int $prop;
}

dumpType((new ReflectionFunction('test1'))->getReturnType());
dumpType((new ReflectionFunction('test2'))->getReturnType());
dumpBCType((new ReflectionFunction('test3'))->getReturnType());
dumpBCType((new ReflectionFunction('test4'))->getReturnType());
dumpType((new ReflectionFunction('test5'))->getReturnType());
dumpBCType((new ReflectionFunction('test6'))->getReturnType());
dumpBCType((new ReflectionFunction('test7'))->getReturnType());

$rc = new ReflectionClass(Test::class);
$rp = $rc->getProperty('prop');
dumpType($rp->getType());

/* Force CE resolution of the property type */

class x {}
$test = new Test;
$test->prop = new x;

$rp = $rc->getProperty('prop');
dumpType($rp->getType());

class y {}
$test->prop = new y;

$rp = $rc->getProperty('prop');
dumpType($rp->getType());

?>
--EXPECTF--
Type X|Y|int|float|false|null:
Allows null: true
  Name: X
  String: X
  Allows Null: false
  Name: Y
  String: Y
  Allows Null: false
  Name: int
  String: int
  Allows Null: false
  Name: float
  String: float
  Allows Null: false
  Name: false
  String: false
  Allows Null: false
  Name: null
  String: null
  Allows Null: true
Type X|Traversable|array|bool:
Allows null: false
  Name: X
  String: X
  Allows Null: false
  Name: Traversable
  String: Traversable
  Allows Null: false
  Name: array
  String: array
  Allows Null: false
  Name: bool
  String: bool
  Allows Null: false
Type ?false:
  Name: false
  String: ?false
  Allows Null: true
Type ?false:
  Name: false
  String: ?false
  Allows Null: true
Type X|Traversable|array|true:
Allows null: false
  Name: X
  String: X
  Allows Null: false
  Name: Traversable
  String: Traversable
  Allows Null: false
  Name: array
  String: array
  Allows Null: false
  Name: true
  String: true
  Allows Null: false
Type ?true:
  Name: true
  String: ?true
  Allows Null: true
Type ?true:
  Name: true
  String: ?true
  Allows Null: true
Type X|Y|int:
Allows null: false
  Name: X
  String: X
  Allows Null: false
  Name: Y
  String: Y
  Allows Null: false
  Name: int
  String: int
  Allows Null: false

Deprecated: Using X as a class name with incorrect case is deprecated, use the correct casing x instead in %s on line %d
Type X|Y|int:
Allows null: false
  Name: X
  String: X
  Allows Null: false
  Name: Y
  String: Y
  Allows Null: false
  Name: int
  String: int
  Allows Null: false

Deprecated: Using X as a class name with incorrect case is deprecated, use the correct casing x instead in %s on line %d

Deprecated: Using Y as a class name with incorrect case is deprecated, use the correct casing y instead in %s on line %d
Type X|Y|int:
Allows null: false
  Name: X
  String: X
  Allows Null: false
  Name: Y
  String: Y
  Allows Null: false
  Name: int
  String: int
  Allows Null: false
