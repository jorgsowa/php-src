--TEST--
Bug #45529 (UTC not properly recognised as timezone identifier while parsing)
--FILE--
<?php
date_default_timezone_set('Europe/Oslo');
$tz1 = new DateTimeZone('UTC');
$tz2 = date_create('UTC')->getTimeZone();
echo $tz1->getName(), PHP_EOL;
echo $tz2->getName(), PHP_EOL;
$d = new DateTime('2008-01-01 12:00:00+0200');
$d->setTimeZone($tz1);
echo $d->format(DATE_ISO8601), PHP_EOL;
$d = new DateTime('2008-01-01 12:00:00+0200');
$d->setTimeZone($tz2);
echo $d->format(DATE_ISO8601), PHP_EOL;
?>
--EXPECTF--
Deprecated: Calling getTimeZone() is deprecated, use the correct casing DateTime::getTimezone() instead in %s on line %d
UTC
UTC

Deprecated: Calling setTimeZone() is deprecated, use the correct casing DateTime::setTimezone() instead in %s on line %d
2008-01-01T10:00:00+0000

Deprecated: Calling setTimeZone() is deprecated, use the correct casing DateTime::setTimezone() instead in %s on line %d
2008-01-01T10:00:00+0000
