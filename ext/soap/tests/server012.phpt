--TEST--
SOAP Server 12: WSDL generation
--SKIPIF--
<?php
if (PHP_OS_FAMILY === "Windows") {
    die("skip currently unsupported on Windows");
}
?>
--EXTENSIONS--
soap
--GET--
WSDL
--FILE--
<?php
function Add($x,$y) {
  return $x+$y;
}

$server = new soapserver(null,array('uri'=>"http://testuri.org"));
$server->addfunction("Add");
$server->handle();
echo "ok\n";
?>
--EXPECTF--
Deprecated: Using soapserver as a class name with incorrect case is deprecated, use the correct casing SoapServer instead in %s on line %d

Deprecated: Calling addfunction() is deprecated, use the correct casing SoapServer::addFunction() instead in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/"><SOAP-ENV:Body><SOAP-ENV:Fault><faultcode>SOAP-ENV:Server</faultcode><faultstring>WSDL generation is not supported yet</faultstring></SOAP-ENV:Fault></SOAP-ENV:Body></SOAP-ENV:Envelope>
