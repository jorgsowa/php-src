--TEST--
catch clause with wrong-case exception class name emits E_DEPRECATED
--FILE--
<?php
class DatabaseException extends RuntimeException {}

try {
    throw new DatabaseException("db error");
} catch (databaseexception $e) {
    echo $e->getMessage() . "\n";
}

try {
    throw new DatabaseException("db error 2");
} catch (DATABASEEXCEPTION $e) {
    echo $e->getMessage() . "\n";
}

try {
    throw new DatabaseException("db error 3");
} catch (DatabaseException $e) {
    echo $e->getMessage() . "\n";
}
?>
--EXPECTF--
Deprecated: Using databaseexception as a class name with incorrect case is deprecated, use the correct casing DatabaseException instead in %s on line %d
db error

Deprecated: Using DATABASEEXCEPTION as a class name with incorrect case is deprecated, use the correct casing DatabaseException instead in %s on line %d
db error 2
db error 3
