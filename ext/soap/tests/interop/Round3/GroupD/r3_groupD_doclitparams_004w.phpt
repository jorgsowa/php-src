--TEST--
SOAP Interop Round3 GroupD Doc Lit Parameters 004 (php/wsdl): echoVoid
--EXTENSIONS--
soap
--INI--
soap.wsdl_cache_enabled=0
--FILE--
<?php
$client = new SoapClient(__DIR__."/round3_groupD_doclitparams.wsdl",array("trace"=>1,"exceptions"=>0));
$client->echoVoid();
echo $client->__getlastrequest();
$HTTP_RAW_POST_DATA = $client->__getlastrequest();
include("round3_groupD_doclitparams.inc");
echo "ok\n";
?>
--EXPECTF--
Deprecated: Calling __getlastrequest() is deprecated, use the correct casing SoapClient::__getLastRequest() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns1="http://soapinterop.org/xsd"><SOAP-ENV:Body><ns1:echoVoid/></SOAP-ENV:Body></SOAP-ENV:Envelope>

Deprecated: Calling __getlastrequest() is deprecated, use the correct casing SoapClient::__getLastRequest() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns1="http://soapinterop.org/xsd"><SOAP-ENV:Body><ns1:echoVoidResponse/></SOAP-ENV:Body></SOAP-ENV:Envelope>
ok
