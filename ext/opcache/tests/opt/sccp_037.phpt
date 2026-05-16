--TEST--
SCCP 037: Memory leak
--INI--
opcache.enable=1
opcache.enable_cli=1
opcache.optimization_level=-1
--FILE--
<?php
[!![[new ERROR]]];
?>
DONE
--EXPECTF--
Deprecated: Using ERROR as a class name with incorrect case is deprecated, use the correct casing Error instead in %s on line %d
DONE
