--TEST--
__construct() with no arguments.
--CREDITS--
Nic Rosental nicrosental@gmail.com
# TestFest Atlanta 2009-5-14
--EXTENSIONS--
dom
--FILE--
<?php
    try {
        $section = new DOMCDataSection();
    } catch (TypeError $e) {
        echo $e->getMessage();
    }
?>
--EXPECTF--
Deprecated: Using DOMCDataSection as a class name with incorrect case is deprecated, use the correct casing DOMCdataSection instead in %s on line %d
DOMCdataSection::__construct() expects exactly 1 argument, 0 given
