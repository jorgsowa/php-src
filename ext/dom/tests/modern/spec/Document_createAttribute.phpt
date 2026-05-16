--TEST--
Document::createAttribute()
--EXTENSIONS--
dom
--FILE--
<?php
require __DIR__ . "/dump_attr.inc";

echo "--- HTML document ---\n";

$dom = Dom\HTMLDocument::createEmpty();
$attr = $dom->createAttribute('foo');
dumpAttr($attr);
$attr = $dom->createAttribute('FoOo');
dumpAttr($attr);

echo "--- XML document ---\n";

$dom = Dom\XMLDocument::createEmpty();
$attr = $dom->createAttribute('foo');
dumpAttr($attr);
$attr = $dom->createAttribute('FoOo');
dumpAttr($attr);
?>
--EXPECTF--
--- HTML document ---

Deprecated: Using DOM\Attr as a class name with incorrect case is deprecated, use the correct casing Dom\Attr instead in %s on line %d
Attr: foo
NULL
string(3) "foo"
NULL
Attr: fooo
NULL
string(4) "fooo"
NULL
--- XML document ---
Attr: foo
NULL
string(3) "foo"
NULL
Attr: FoOo
NULL
string(4) "FoOo"
NULL
