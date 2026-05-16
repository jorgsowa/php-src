--TEST--
Bug #36756 (DOMDocument::removeChild corrupts node)
--EXTENSIONS--
dom
--FILE--
<?php

/* Node is preserved from removeChild */
$dom = new DOMDocument();
$dom->loadXML('<root><child/></root>');
$xpath = new DOMXpath($dom);
$node = $xpath->query('/root')->item(0);
echo $node->nodeName . "\n";
$dom->removeChild($GLOBALS['dom']->firstChild);
echo "nodeType: " . $node->nodeType . "\n";
/* Node gets destroyed during removeChild */
$dom->loadXML('<root><child/></root>');
$xpath = new DOMXpath($dom);
$node = $xpath->query('//child')->item(0);
echo $node->nodeName . "\n";
$GLOBALS['dom']->removeChild($GLOBALS['dom']->firstChild);

echo "nodeType: " . $node->nodeType . "\n";

?>
--EXPECTF--
Deprecated: Using DOMXpath as a class name with incorrect case is deprecated, use the correct casing DOMXPath instead in %s on line %d
root
nodeType: 1

Deprecated: Using DOMXpath as a class name with incorrect case is deprecated, use the correct casing DOMXPath instead in %s on line %d
child
nodeType: 1
