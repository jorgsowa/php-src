--TEST--
Delayed freeing should not cause a UAF
--EXTENSIONS--
dom
--FILE--
<?php
$xml = new DomDocument();
$d = $xml->createElement("div");
$d->appendChild($b = $xml->createElement("b"));
$ret = $d->appendChild($xml->createElement("xxx"));
echo $xml->saveXML($d), "\n";
unset($d);

echo $ret->textContent, "Done\n";
?>
--EXPECTF--
Deprecated: Using DomDocument as a class name with incorrect case is deprecated, use the correct casing DOMDocument instead in %s on line %d
<div><b/><xxx/></div>
Done
