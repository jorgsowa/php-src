--TEST--
TokenList: foreach by ref
--EXTENSIONS--
dom
--FILE--
<?php

$dom = DOM\XMLDocument::createFromString('<root class="A B C"/>');

try {
    foreach ($dom->documentElement->classList as &$class) {
    }
} catch (Error $e) {
    echo $e->getMessage(), "\n";
}

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d
An iterator cannot be used with foreach by reference
