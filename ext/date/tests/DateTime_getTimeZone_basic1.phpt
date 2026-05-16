--TEST--
Test DateTime::getTimezone() function : basic functionality
--FILE--
<?php
echo "*** Testing DateTime::getTimezone() : basic functionality ***\n";

date_default_timezone_set("Europe/London");
$object = new DateTime("2009-01-30 17:57:32");
var_dump( $object->getTimeZone()->getName() );


date_default_timezone_set("America/New_York");
$object = new DateTime("2009-01-30 17:57:32");
var_dump( $object->getTimeZone()->getName() );

$la_time = new DateTimeZone("America/Los_Angeles");
$object->setTimeZone($la_time);
var_dump( $object->getTimeZone()->getName() );

?>
--EXPECTF--
*** Testing DateTime::getTimezone() : basic functionality ***

Deprecated: Calling getTimeZone() is deprecated, use the correct casing DateTime::getTimezone() instead in %s on line %d
string(13) "Europe/London"

Deprecated: Calling getTimeZone() is deprecated, use the correct casing DateTime::getTimezone() instead in %s on line %d
string(16) "America/New_York"

Deprecated: Calling setTimeZone() is deprecated, use the correct casing DateTime::setTimezone() instead in %s on line %d

Deprecated: Calling getTimeZone() is deprecated, use the correct casing DateTime::getTimezone() instead in %s on line %d
string(19) "America/Los_Angeles"
