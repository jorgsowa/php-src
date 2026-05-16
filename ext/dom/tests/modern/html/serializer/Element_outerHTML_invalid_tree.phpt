--TEST--
Test reading Element::$outerHTML on HTML documents - invalid tree variation
--EXTENSIONS--
dom
--CREDITS--
Dennis Snell
--FILE--
<?php

$dom = Dom\HTMLDocument::createFromString('<a href="#one"><p>Link</p></a>', LIBXML_NOERROR);
$p = $dom->body->querySelector('p');
$p->outerHTML = '<a href="#two">Another Link</a>';
echo $dom->saveHTML();

?>
--EXPECTF--
Deprecated: Calling saveHTML() is deprecated, use the correct casing Dom\HTMLDocument::saveHtml() instead in %s on line %d
<html><head></head><body><a href="#one"><a href="#two">Another Link</a></a></body></html>
