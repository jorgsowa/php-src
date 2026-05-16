--TEST--
libxml_set_streams_context()
--EXTENSIONS--
dom
--FILE--
<?php

$ctxs = array(
    NULL,
    'bogus',
    123,
    new stdclass,
    array('a'),
    stream_context_create(),
);

foreach ($ctxs as $ctx) {
    try {
        var_dump(libxml_set_streams_context($ctx));
    } catch (TypeError $e) {
        echo $e->getMessage(), "\n";
    }
    $dom = new DOMDocument();
    var_dump($dom->load(__DIR__.'/test.xml'));
}

echo "Done\n";

?>
--EXPECTF--
Deprecated: Using stdclass as a class name with incorrect case is deprecated, use the correct casing stdClass instead in %s on line %d
libxml_set_streams_context(): Argument #1 ($context) must be of type resource, null given
bool(true)
libxml_set_streams_context(): Argument #1 ($context) must be of type resource, string given
bool(true)
libxml_set_streams_context(): Argument #1 ($context) must be of type resource, int given
bool(true)
libxml_set_streams_context(): Argument #1 ($context) must be of type resource, stdClass given
bool(true)
libxml_set_streams_context(): Argument #1 ($context) must be of type resource, array given
bool(true)
NULL
bool(true)
Done
