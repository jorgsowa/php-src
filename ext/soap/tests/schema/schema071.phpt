--TEST--
SOAP XML Schema 71: SOAP 1.1 Array (document style)
--EXTENSIONS--
soap
xml
--FILE--
<?php
include "test_schema.inc";
$schema = <<<EOF
    <complexType name="testType">
        <complexContent>
            <restriction base="SOAP-ENC:Array">
        <attribute ref="SOAP-ENC:arrayType" wsdl:arrayType="int[]"/>
        </restriction>
    </complexContent>
    </complexType>
EOF;
test_schema($schema,'type="tns:testType"',array(123,123.5),'document','literal');
echo "ok";
?>
--EXPECTF--
Deprecated: Calling addfunction() is deprecated, use the correct casing SoapServer::addFunction() instead in %s on line %d

Deprecated: Calling __getlastrequest() is deprecated, use the correct casing SoapClient::__getLastRequest() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsd="http://www.w3.org/2001/XMLSchema"><SOAP-ENV:Body><testParam><xsd:int>123</xsd:int><xsd:int>123</xsd:int></testParam></SOAP-ENV:Body></SOAP-ENV:Envelope>
ok
