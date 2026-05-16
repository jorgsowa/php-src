--TEST--
Test percent-decoding of reserved characters in the path
--EXTENSIONS--
uri
--FILE--
<?php

$uri = new Uri\Rfc3986\Uri("https://example.com/foo/bar%2Fbaz");

var_dump($uri->toRawString());
var_dump($uri->getPath());
var_dump($uri->getRawPath());

$url = new Uri\Whatwg\Url("https://example.com/foo/bar%2Fbaz");

var_dump($url->toAsciiString());
var_dump($url->getPath());

?>
--EXPECTF--
string(33) "https://example.com/foo/bar%2Fbaz"
string(14) "/foo/bar%2Fbaz"
string(14) "/foo/bar%2Fbaz"

Deprecated: Using Uri\Whatwg\Url as a class name with incorrect case is deprecated, use the correct casing Uri\WhatWg\Url instead in %s on line %d
string(33) "https://example.com/foo/bar%2Fbaz"
string(14) "/foo/bar%2Fbaz"
