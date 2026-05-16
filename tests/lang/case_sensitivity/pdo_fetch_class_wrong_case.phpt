--TEST--
PDOStatement::setFetchMode(FETCH_CLASS) with wrong-case class name emits E_DEPRECATED
--EXTENSIONS--
pdo
pdo_sqlite
--FILE--
<?php
class MyRow {
    public mixed $a = null;
}

$pdo = new PDO("sqlite::memory:");
$pdo->exec("CREATE TABLE t (a TEXT)");
$pdo->exec("INSERT INTO t VALUES ('hello')");

$stmt = $pdo->prepare("SELECT a FROM t");
$stmt->execute();
$stmt->setFetchMode(PDO::FETCH_CLASS, "MYROW");
$row = $stmt->fetch();
echo get_class($row) . "\n";
echo $row->a . "\n";
?>
--EXPECTF--
Deprecated: Using MYROW as a class name with incorrect case is deprecated, use the correct casing MyRow instead in %s on line %d
MyRow
hello
