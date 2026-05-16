--TEST--
SCCP 015: Conditional Constant Propagation of non-escaping object properties on PHI and Rewinding
--INI--
opcache.enable=1
opcache.enable_cli=1
opcache.optimization_level=-1
--EXTENSIONS--
opcache
--FILE--
<?php
function loadEntities($entity_information) {
    $entity_types = new StdClass();
    $entity_types->b = 0;
    foreach ($entity_information as $ex) {
        var_dump((bool)$entity_types->b);
        foreach ($entity_information as $info) {
            $entity_types->b = 1;
        }
    }
}

loadEntities(array("first", "second"));
?>
--EXPECTF--
Deprecated: Using StdClass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
bool(false)
bool(true)
