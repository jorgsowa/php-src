--TEST--
FR #79344 (xmlwriter_write_attribute_ns: $prefix should be nullable)
--EXTENSIONS--
xmlwriter
--FILE--
<?php
$writer = new XMLWriter;
$writer->openMemory();
$writer->setIndent(true);
$writer->startElement('foo');

$writer->writeAttributeNS(null, 'test1', null, 'test1');
$writer->startAttributeNS(null, 'test2', null);
$writer->text('test2');
$writer->endAttribute();

$writer->endElement();
echo $writer->outputMemory();
?>
--EXPECTF--
Deprecated: Calling writeAttributeNS() is deprecated, use the correct casing XMLWriter::writeAttributeNs() instead in %s on line %d

Deprecated: Calling startAttributeNS() is deprecated, use the correct casing XMLWriter::startAttributeNs() instead in %s on line %d
<foo test1="test1" test2="test2"/>
