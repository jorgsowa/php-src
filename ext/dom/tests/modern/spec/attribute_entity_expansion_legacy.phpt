--TEST--
Attribute entity expansion in a legacy document
--EXTENSIONS--
dom
--FILE--
<?php
$doc = new DOMDocument;
$elt = $doc->createElement('elt');
$doc->appendChild($elt);
$elt->setAttribute('a','&');
print $doc->saveXml($elt) . "\n";

$attr = $elt->getAttributeNode('a');
$attr->value = '&amp;';
print "$attr->value\n";
print $doc->saveXml($elt) . "\n";

$attr->removeChild($attr->firstChild);
print $doc->saveXml($elt) . "\n";

// Note: since libxml2 commit aca16fb3d45e0b2c45364ffc1cea8eb4abaca87d this no longer explicitly warns. This seems intentional.
@$attr->nodeValue = '&';
print "$attr->nodeValue\n";
print $doc->saveXml($elt) . "\n";

$attr->nodeValue = '&amp;';
print "$attr->nodeValue\n";
print $doc->saveXml($elt) . "\n";

$elt->removeAttributeNode($attr);
$elt->setAttributeNS('http://www.w3.org/2000/svg', 'svg:id','&amp;');
print $doc->saveXml($elt) . "\n";

$attr = $elt->getAttributeNodeNS('http://www.w3.org/2000/svg', 'id');
$attr->value = '&lt;&amp;';
print "$attr->value\n";
print $doc->saveXml($elt) . "\n";
?>
--EXPECTF--
Deprecated: Calling saveXml() is deprecated, use the correct casing DOMDocument::saveXML() instead in %s on line %d
<elt a="&amp;"/>
&

Deprecated: Calling saveXml() is deprecated, use the correct casing DOMDocument::saveXML() instead in %s on line %d
<elt a="&amp;"/>

Deprecated: Calling saveXml() is deprecated, use the correct casing DOMDocument::saveXML() instead in %s on line %d
<elt a=""/>


Deprecated: Calling saveXml() is deprecated, use the correct casing DOMDocument::saveXML() instead in %s on line %d
<elt a=""/>
&

Deprecated: Calling saveXml() is deprecated, use the correct casing DOMDocument::saveXML() instead in %s on line %d
<elt a="&amp;"/>

Deprecated: Calling saveXml() is deprecated, use the correct casing DOMDocument::saveXML() instead in %s on line %d
<elt xmlns:svg="http://www.w3.org/2000/svg" svg:id="&amp;amp;"/>
<&

Deprecated: Calling saveXml() is deprecated, use the correct casing DOMDocument::saveXML() instead in %s on line %d
<elt xmlns:svg="http://www.w3.org/2000/svg" svg:id="&lt;&amp;"/>
