--TEST--
Implementing an interface with wrong-case name emits E_DEPRECATED
--FILE--
<?php
interface MyInterface {
    public function doSomething(): void;
}

class Child implements MYINTERFACE {
    public function doSomething(): void {}
}

echo "done\n";
?>
--EXPECTF--
Deprecated: Using MYINTERFACE as a class name with incorrect case is deprecated, use the correct casing MyInterface instead in %s on line %d
done
