--TEST--
array test
--FILE--
<?php
class ArrayIteratorEx extends ArrayIterator
{
    function current(): mixed
    {
        return ArrayIterator::current();
    }
}
$it = new ArrayIteratorEx(range(0,3));

foreach(new IteratorIterator($it) )
{
    echo 'a';
}



?>
--EXPECT--


