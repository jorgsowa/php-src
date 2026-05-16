--TEST--
GH-17485 (Self-closing tag on void elements shouldn't be a parse error/warning in \Dom\HTMLDocument)
--EXTENSIONS--
dom
--FILE--
<?php
$Data = "<!DOCTYPE HTML>\n<br />\n<input />";
$Document = \Dom\HTMLDocument::createFromString( $Data );
echo $Document->saveHTML();
?>
--EXPECTF--
Deprecated: Calling saveHTML() is deprecated, use the correct casing Dom\HTMLDocument::saveHtml() instead in %s on line %d
<!DOCTYPE html><html><head></head><body><br>
<input></body></html>
