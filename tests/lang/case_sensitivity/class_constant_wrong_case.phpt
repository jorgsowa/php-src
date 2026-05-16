--TEST--
Class constant and enum case access with wrong-cased class name emits E_DEPRECATED
--FILE--
<?php
class Status {
    const ACTIVE = 1;
}

enum Suit: string {
    case Hearts = 'H';
}

// Correct casing - no warning
echo Status::ACTIVE, "\n";
echo Suit::Hearts->value, "\n";

// Wrong-cased class name on a class constant (compile-time resolved)
echo STATUS::ACTIVE, "\n";

// Wrong-cased enum name on an enum case (resolved at runtime)
echo SUIT::Hearts->value, "\n";
?>
--EXPECTF--
Deprecated: Using STATUS as a class name with incorrect case is deprecated, use the correct casing Status instead in %s on line 15
1
H
1

Deprecated: Using SUIT as a class name with incorrect case is deprecated, use the correct casing Suit instead in %s on line 18
H
