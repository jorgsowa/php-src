--TEST--
GH-18090 (Svg attributes and tag names are being lowercased)
--EXTENSIONS--
dom
--FILE--
<?php
echo \Dom\HTMLDocument::createFromString('<html><body><svg VIEWBOX="1 2 3 4"></svg></html>', LIBXML_NOERROR)->saveHTML(), "\n";

echo \Dom\HTMLDocument::createFromString('<html><body CLASS="no"><svg VIEWBOX="1 2 3 4"><feSpotLight x="10" y="10" z="50" pointsAtX="100" pointsAtY="100" limitingConeAngle="
10" /></svg></html>', LIBXML_NOERROR)->saveHTML(), "\n";

echo \Dom\HTMLDocument::createFromString('<html><body><svg VIEWBOX="1 2 3 4"></svg></html>', LIBXML_NOERROR)->querySelector('svg')->attributes[0]->name, "\n";
?>
--EXPECTF--
Deprecated: Calling saveHTML() is deprecated, use the correct casing Dom\HTMLDocument::saveHtml() instead in %s on line %d
<html><head></head><body><svg viewBox="1 2 3 4"></svg></body></html>

Deprecated: Calling saveHTML() is deprecated, use the correct casing Dom\HTMLDocument::saveHtml() instead in %s on line %d
<html><head></head><body class="no"><svg viewBox="1 2 3 4"><feSpotLight x="10" y="10" z="50" pointsAtX="100" pointsAtY="100" limitingConeAngle="
10"></feSpotLight></svg></body></html>
viewBox
