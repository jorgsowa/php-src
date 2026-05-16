--TEST--
GH-15570 (Segmentation fault (access null pointer) in ext/dom/html5_serializer.c)
--CREDITS--
YuanchengJiang
--EXTENSIONS--
dom
--FILE--
<?php
$html = <<<HTML
<head>
</html>
HTML;
$dom = Dom\HTMLDocument::createFromString($html, LIBXML_NOERROR);
$a = $dom->head->firstChild->cloneNode(false);
var_dump($dom->saveHTML($a));
?>
--EXPECTF--
Deprecated: Calling saveHTML() is deprecated, use the correct casing Dom\HTMLDocument::saveHtml() instead in %s on line %d
string(1) "
"
