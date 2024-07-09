--TEST--
array test
--FILE--
<?php

foreach([1,2,3,4]) {
    echo 'a';
}


foreach([]) {
    echo 'a';
}

$range = range(1,5);

foreach($range) {
    echo 'c';
}

const RANGE = [1, 2, 3];

foreach(RANGE) {
    echo 'c';
}



?>
--EXPECT--
aaaacccccccc
