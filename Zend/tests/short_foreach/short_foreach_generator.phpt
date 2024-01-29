--TEST--
array test
--FILE--
<?php

function generator() {
    yield from [0,1,2,3];
}

foreach(generator()) {
    echo 'a';
}

function generator2() {
    for($i=0;$i<0;$i++) {
        yield $i;
    }
}

foreach(generator2()) {
    echo 'b';
}


?>
--EXPECT--

