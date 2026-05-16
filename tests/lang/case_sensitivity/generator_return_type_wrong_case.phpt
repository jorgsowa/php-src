--TEST--
Generator return type with wrong-cased class name emits E_DEPRECATED
--FILE--
<?php
// Generator-compatible return types (Generator, Iterator, Traversable) are
// matched case-insensitively; a wrong-cased name emits one deprecation per
// function, regardless of how many yields it contains.

function genWrong(): GENERATOR {
    yield 1;
}

function iterWrong(): iterator {
    yield 1;
}

function travWrong(): TRAVERSABLE {
    yield 1;
    yield 2;
}

function fromWrong(): IterATOR {
    yield from [1, 2];
}

class C {
    public function methodWrong(): traversable {
        yield 1;
    }
}

// Correct casing — no deprecation.
function genOk(): Generator {
    yield 1;
}

echo "done\n";
?>
--EXPECTF--
Deprecated: Using GENERATOR as a class name with incorrect case is deprecated, use the correct casing Generator instead in %s on line %d

Deprecated: Using iterator as a class name with incorrect case is deprecated, use the correct casing Iterator instead in %s on line %d

Deprecated: Using TRAVERSABLE as a class name with incorrect case is deprecated, use the correct casing Traversable instead in %s on line %d

Deprecated: Using IterATOR as a class name with incorrect case is deprecated, use the correct casing Iterator instead in %s on line %d

Deprecated: Using traversable as a class name with incorrect case is deprecated, use the correct casing Traversable instead in %s on line %d
done
