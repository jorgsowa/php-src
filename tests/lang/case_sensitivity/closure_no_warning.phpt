--TEST--
Closures do not emit case deprecation warning
--FILE--
<?php
$fn = function() { return "closure"; };
echo $fn() . "\n";

$obj = new class {
    public function __invoke() { return "invokable"; }
};
echo $obj() . "\n";
?>
--EXPECT--
closure
invokable
