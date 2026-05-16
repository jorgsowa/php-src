--TEST--
Bug #76770 'U' modifier in 'datetime::createFromFormat' adds seconds to other specifiers
--FILE--
<?php
var_dump(datetime::createFromFormat('U H', '3600 01')->getTimestamp());
?>
--EXPECTF--
Deprecated: Using datetime as a class name with incorrect case is deprecated, use the correct casing DateTime instead in %s on line %d
int(3600)
