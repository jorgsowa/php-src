--TEST--
Bug #41287 (Writing empty tags with Xmlwriter::WriteElement[ns])
--EXTENSIONS--
xmlwriter
--FILE--
<?php
$xml = new XmlWriter();
$xml->openMemory();
$xml->setIndent(true);
$xml->startDocument();
$xml->startElement('test');
$xml->writeElement('foo', null);
$xml->writeElement('foo2', "");
$xml->writeElement('foo3');
$xml->startElement('bar');
$xml->endElement();
$xml->endElement();
print $xml->flush(true);

print "\n";

$xw = new XMLWriter();
$xw->openMemory();
$xw->setIndent(true);
$xw->startDocument();
$xw->startElementNS('test', 'test', 'urn:x-test:');
$xw->writeElementNS('test', 'foo', null, '');
$xw->writeElementNS(null, 'bar', 'urn:x-test:', '');
$xw->writeElementNS(null, 'bar', 'urn:x-test:', NULL);
$xw->writeElementNS(null, 'bar', 'urn:x-test:');
$xw->writeElementNS(null, 'bar', '', '');
$xw->endElement();
$xw->endDocument();
print $xw->flush(true);
?>
--EXPECTF--
Deprecated: Using XmlWriter as a class name with incorrect case is deprecated, use the correct casing XMLWriter instead in %s on line %d
<?xml version="1.0"?>
<test>
 <foo/>
 <foo2></foo2>
 <foo3/>
 <bar/>
</test>


Deprecated: Calling startElementNS() is deprecated, use the correct casing XMLWriter::startElementNs() instead in %s on line %d

Deprecated: Calling writeElementNS() is deprecated, use the correct casing XMLWriter::writeElementNs() instead in %s on line %d

Deprecated: Calling writeElementNS() is deprecated, use the correct casing XMLWriter::writeElementNs() instead in %s on line %d

Deprecated: Calling writeElementNS() is deprecated, use the correct casing XMLWriter::writeElementNs() instead in %s on line %d

Deprecated: Calling writeElementNS() is deprecated, use the correct casing XMLWriter::writeElementNs() instead in %s on line %d

Deprecated: Calling writeElementNS() is deprecated, use the correct casing XMLWriter::writeElementNs() instead in %s on line %d
<?xml version="1.0"?>
<test:test xmlns:test="urn:x-test:">
 <test:foo></test:foo>
 <bar xmlns="urn:x-test:"></bar>
 <bar xmlns="urn:x-test:"/>
 <bar xmlns="urn:x-test:"/>
 <bar xmlns=""></bar>
</test:test>
