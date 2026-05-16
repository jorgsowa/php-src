--TEST--
TokenList: iteration 02
--EXTENSIONS--
dom
--FILE--
<?php

$dom = DOM\XMLDocument::createFromString('<root class="A B C D E F"/>');
$list = $dom->documentElement->classList;

foreach ($list as $i => $item) {
    var_dump($i, $item);
    echo "==========\n";
    foreach ($list as $i2 => $item2) {
        var_dump($i2, $item2);
    }
    echo "==========\n";
}

?>
--EXPECTF--
Deprecated: Using DOM\XMLDocument as a class name with incorrect case is deprecated, use the correct casing Dom\XMLDocument instead in %s on line %d
int(0)
string(1) "A"
==========
int(0)
string(1) "A"
int(1)
string(1) "B"
int(2)
string(1) "C"
int(3)
string(1) "D"
int(4)
string(1) "E"
int(5)
string(1) "F"
==========
int(1)
string(1) "B"
==========
int(0)
string(1) "A"
int(1)
string(1) "B"
int(2)
string(1) "C"
int(3)
string(1) "D"
int(4)
string(1) "E"
int(5)
string(1) "F"
==========
int(2)
string(1) "C"
==========
int(0)
string(1) "A"
int(1)
string(1) "B"
int(2)
string(1) "C"
int(3)
string(1) "D"
int(4)
string(1) "E"
int(5)
string(1) "F"
==========
int(3)
string(1) "D"
==========
int(0)
string(1) "A"
int(1)
string(1) "B"
int(2)
string(1) "C"
int(3)
string(1) "D"
int(4)
string(1) "E"
int(5)
string(1) "F"
==========
int(4)
string(1) "E"
==========
int(0)
string(1) "A"
int(1)
string(1) "B"
int(2)
string(1) "C"
int(3)
string(1) "D"
int(4)
string(1) "E"
int(5)
string(1) "F"
==========
int(5)
string(1) "F"
==========
int(0)
string(1) "A"
int(1)
string(1) "B"
int(2)
string(1) "C"
int(3)
string(1) "D"
int(4)
string(1) "E"
int(5)
string(1) "F"
==========
