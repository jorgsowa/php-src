--TEST--
SOAP 1.2: T33 nonexistentMethod
--EXTENSIONS--
soap
--FILE--
<?php
$HTTP_RAW_POST_DATA = <<<EOF
<?xml version='1.0' ?>
<env:Envelope xmlns:env="http://www.w3.org/2003/05/soap-envelope">
  <env:Body>
    <test:DoesNotExist xmlns:test="http://example.org/ts-tests">
  </test:DoesNotExist>
 </env:Body>
</env:Envelope>
EOF;
include "soap12-test.inc";
?>
--EXPECTF--
Deprecated: Using soapserver as a class name with incorrect case is deprecated, use the correct casing SoapServer instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<env:Envelope xmlns:env="http://www.w3.org/2003/05/soap-envelope"><env:Body><env:Fault><env:Code><env:Value>rpc:ProcedureNotPresent</env:Value></env:Code><env:Reason><env:Text xml:lang="en">Procedure not present</env:Text></env:Reason></env:Fault></env:Body></env:Envelope>
