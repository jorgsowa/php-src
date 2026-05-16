--TEST--
PDO::ATTR_STATEMENT_CLASS with wrong-case class name emits E_DEPRECATED
--EXTENSIONS--
pdo
pdo_sqlite
--FILE--
<?php
class MyStatement extends PDOStatement {
    public int $counter = 0;
}

$pdo = new PDO("sqlite::memory:");
$pdo->setAttribute(PDO::ATTR_STATEMENT_CLASS, ["MYSTATEMENT"]);
$stmt = $pdo->query("SELECT 1");
echo get_class($stmt) . "\n";
?>
--EXPECTF--
Deprecated: Using MYSTATEMENT as a class name with incorrect case is deprecated, use the correct casing MyStatement instead in %s on line %d
MyStatement
