--TEST--
SOAP 1.2: T32 echoHeader
--EXTENSIONS--
soap
--FILE--
<?php
$HTTP_RAW_POST_DATA = <<<EOF
<?xml version='1.0' ?>
<env:Envelope xmlns:env="http://www.w3.org/2003/05/soap-envelope">
  <env:Header>
    <test:requiredHeader xmlns:test="http://example.org/ts-tests"
          env:mustUnderstand="true">foo</test:requiredHeader>
  </env:Header>
  <env:Body>
    <test:echoHeader xmlns:test="http://example.org/ts-tests">
    </test:echoHeader>
  </env:Body>
</env:Envelope>
EOF;
include "soap12-test.inc";
?>
--EXPECTF--
Deprecated: Using soapserver as a class name with incorrect case is deprecated, use the correct casing SoapServer instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<env:Envelope xmlns:env="http://www.w3.org/2003/05/soap-envelope" xmlns:ns1="http://example.org/ts-tests"><env:Body><ns1:echoHeaderResponse>foo</ns1:echoHeaderResponse></env:Body></env:Envelope>
ok
