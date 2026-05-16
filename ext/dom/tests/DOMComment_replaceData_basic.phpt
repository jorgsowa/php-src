--TEST--
Test replacing data into a DOMComment basic test
--CREDITS--
Andrew Larssen <al@larssen.org>
London TestFest 2008
--EXTENSIONS--
dom
--FILE--
<?php

$dom = new DomDocument();
$comment = $dom->createComment('test-comment');
$comment->replaceData(4,1,'replaced');
$dom->appendChild($comment);
echo $dom->saveXML();

// Replaces rest of string if count is greater than length of existing string
$dom = new DomDocument();
$comment = $dom->createComment('test-comment');
$comment->replaceData(0,50,'replaced');
$dom->appendChild($comment);
echo $dom->saveXML();

?>
--EXPECTF--
Deprecated: Using DomDocument as a class name with incorrect case is deprecated, use the correct casing DOMDocument instead in %s on line %d
<?xml version="1.0"?>
<!--testreplacedcomment-->

Deprecated: Using DomDocument as a class name with incorrect case is deprecated, use the correct casing DOMDocument instead in %s on line %d
<?xml version="1.0"?>
<!--replaced-->
