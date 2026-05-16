--TEST--
Bug #36743 (In a class extending XMLReader array properties are not writable)
--EXTENSIONS--
xmlreader
--FILE--
<?php

class Test extends XMLReader
{
    private $testArr = array();
    public function __construct()
    {
        $this->testArr[] = 1;
        var_dump($this->testArr);
    }
}

$t = new test;

echo "Done\n";
?>
--EXPECTF--
Deprecated: Using test as a class name with incorrect case is deprecated, use the correct casing Test instead in %s on line %d
array(1) {
  [0]=>
  int(1)
}
Done
