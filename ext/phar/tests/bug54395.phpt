--TEST--
Bug #54395 (Phar::mount() crashes when calling with wrong parameters)
--EXTENSIONS--
phar
--FILE--
<?php

try {
    phar::mount(1,1);
} catch (Exception $e) {
    var_dump($e->getMessage());
}

?>
--EXPECTF--
Deprecated: Using phar as a class name with incorrect case is deprecated, use the correct casing Phar instead in %s on line %d
string(25) "Mounting of 1 to 1 failed"
