--TEST--
Bug GH-9891 (DateTime modify with unixtimestamp (@) must work like setTimestamp)
--FILE--
<?php
$m = new DateTime('2022-12-20 14:30:25', new DateTimeZone('Europe/Paris'));
$m->modify('@1234567890');
var_dump($m->getTimeStamp());

echo "=======\n";

$a = new DateTime('2022-11-01 13:30:00', new DateTimezone('America/Lima'));
$b = clone $a;
echo '$a: ', $a->format(DateTime::ATOM), "\n";
echo '$b: ', $b->format(DateTime::ATOM), "\n";
echo '$a: @', $a->getTimestamp(), "\n";
echo '$b: setTimestamp(', $b->getTimestamp(), ")\n";
$a->modify('@' . $a->getTimestamp());
$b->setTimestamp($b->getTimestamp());
echo '$a: ', $a->format(DateTime::ATOM), "\n";
echo '$b: ', $b->format(DateTime::ATOM), "\n";
?>
--EXPECTF--
Deprecated: Calling getTimeStamp() is deprecated, use the correct casing DateTime::getTimestamp() instead in %s on line %d
int(1234567890)
=======

Deprecated: Using DateTimezone as a class name with incorrect case is deprecated, use the correct casing DateTimeZone instead in %s on line %d
$a: 2022-11-01T13:30:00-05:00
$b: 2022-11-01T13:30:00-05:00
$a: @1667327400
$b: setTimestamp(1667327400)
$a: 2022-11-01T18:30:00+00:00
$b: 2022-11-01T13:30:00-05:00
