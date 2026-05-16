--TEST--
unserialize() with wrong-case enum name emits E_DEPRECATED
--FILE--
<?php
enum Suit: string {
    case Hearts = 'H';
    case Spades = 'S';
}

// Correct case — no deprecation
$s = unserialize('E:11:"Suit:Hearts";');
echo $s->name . "\n";

// Wrong case — E_DEPRECATED
$s2 = unserialize('E:11:"SUIT:Hearts";');
echo $s2->name . "\n";
?>
--EXPECTF--
Hearts

Deprecated: Using SUIT as a class name with incorrect case is deprecated, use the correct casing Suit instead in %s on line %d
Hearts
