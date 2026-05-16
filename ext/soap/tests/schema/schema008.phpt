--TEST--
SOAP XML Schema 8: simpleType/restriction (anonymous, inside an ellement)
--EXTENSIONS--
soap
xml
--FILE--
<?php
include "test_schema.inc";
$schema = <<<EOF
<element name="testElement">
    <simpleType>
        <restriction>
            <simpleType name="testType2">
            <restriction base="xsd:int"/>
        </simpleType>
      </restriction>
    </simpleType>
</element>
EOF;
test_schema($schema,'element="tns:testElement"',123.5);
echo "ok";
?>
--EXPECTF--
Deprecated: Calling addfunction() is deprecated, use the correct casing SoapServer::addFunction() instead in %s on line %d

Deprecated: Calling __getlastrequest() is deprecated, use the correct casing SoapClient::__getLastRequest() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns1="http://test-uri/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:SOAP-ENC="http://schemas.xmlsoap.org/soap/encoding/" SOAP-ENV:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><SOAP-ENV:Body><ns1:test><testParam xsi:type="ns1:testElement">123</testParam></ns1:test></SOAP-ENV:Body></SOAP-ENV:Envelope>
int(123)
ok
