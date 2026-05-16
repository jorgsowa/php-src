--TEST--
Bug #31422 (No Error-Logging on SoapServer-Side)
--EXTENSIONS--
soap
--INI--
log_errors=1
error_log=
--FILE--
<?php
function Add($x,$y) {
    fopen();
    user_error("Hello", E_USER_ERROR);
  return $x+$y;
}

$server = new SoapServer(null,array('uri'=>"http://testuri.org"));
$server->addfunction("Add");

$HTTP_RAW_POST_DATA = <<<EOF
<?xml version="1.0" encoding="ISO-8859-1"?>
<SOAP-ENV:Envelope
  SOAP-ENV:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"
  xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:si="http://soapinterop.org/xsd">
  <SOAP-ENV:Body>
    <ns1:Add xmlns:ns1="http://testuri.org">
      <x xsi:type="xsd:int">22</x>
      <y xsi:type="xsd:int">33</y>
    </ns1:Add>
  </SOAP-ENV:Body>
</SOAP-ENV:Envelope>
EOF;

$server->handle($HTTP_RAW_POST_DATA);
echo "ok\n";
?>
--EXPECTF--
PHP Deprecated:  Calling addfunction() is deprecated, use the correct casing SoapServer::addFunction() instead in %s on line %d

Deprecated: Calling addfunction() is deprecated, use the correct casing SoapServer::addFunction() instead in %s on line %d
PHP Warning:  Cannot modify header information - headers already sent by (output started at %s:%d) in %s on line %d
PHP Warning:  Cannot modify header information - headers already sent by (output started at %s:%d) in %s on line %d
PHP Warning:  Cannot modify header information - headers already sent by (output started at %s:%d) in %s on line %d
<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/"><SOAP-ENV:Body><SOAP-ENV:Fault><faultcode>SOAP-ENV:Server</faultcode><faultstring>fopen() expects at least 2 arguments, 0 given</faultstring></SOAP-ENV:Fault></SOAP-ENV:Body></SOAP-ENV:Envelope>
ok
