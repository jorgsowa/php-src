--TEST--
XMLWriter: libxml2 XML Writer, Write Raw
--CREDITS--
Mark Baker mark@lange.demon.co.uk at the PHPNW2017 Conference for PHP Testfest 2017
--EXTENSIONS--
xmlwriter
--FILE--
<?php

$cDataString = "<cdataElement><![CDATA[Text for inclusion within CData tags can include characters like <, >, &, and quotes like ' and \"]]></cdataElement>";
$xmlWriter = new XmlWriter();
$xmlWriter->openMemory();

$xmlWriter->startDocument('1.0', 'UTF-8');
$xmlWriter->startElement('myDocumentRoot');
$xmlWriter->startElement('myElement');
// CData output
$xmlWriter->writeRaw($cDataString);
// end the document and output
$xmlWriter->endElement();
$xmlWriter->endElement();

echo $xmlWriter->outputMemory(true);

?>
--EXPECTF--
Deprecated: Using XmlWriter as a class name with incorrect case is deprecated, use the correct casing XMLWriter instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<myDocumentRoot><myElement><cdataElement><![CDATA[Text for inclusion within CData tags can include characters like <, >, &, and quotes like ' and "]]></cdataElement></myElement></myDocumentRoot>
