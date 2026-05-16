--TEST--
DOMDocument::saveHTML() vs DOMDocument::saveXML()
--EXTENSIONS--
dom
--FILE--
<?php
$d = new DOMDocument();
$str = <<<EOD
<html>
<head>
</head>
<body>
<p>Hi.<br/>there</p>
</body>
</html>
EOD;
$d->loadHTML($str);
$e = $d->getElementsByTagName("p");
$e = $e->item(0);
echo $d->saveXml($e),"\n";
echo $d->saveHtml($e),"\n";
?>
--EXPECTF--
Deprecated: Calling saveXml() is deprecated, use the correct casing DOMDocument::saveXML() instead in %s on line %d
<p>Hi.<br/>there</p>

Deprecated: Calling saveHtml() is deprecated, use the correct casing DOMDocument::saveHTML() instead in %s on line %d
<p>Hi.<br>there</p>
