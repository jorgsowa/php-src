--TEST--
SoapServer classmap with wrong-case class name emits E_DEPRECATED
--EXTENSIONS--
soap
--INI--
soap.wsdl_cache_enabled=0
--FILE--
<?php
/* SOAP installs its own error handler during handle() that suppresses the
 * display of non-fatal errors, and handle() buffers all output, so the
 * deprecation is captured with a user error handler and printed afterwards. */
$deprecations = [];
set_error_handler(function (int $errno, string $errstr) use (&$deprecations): bool {
    if ($errno === E_DEPRECATED) {
        $deprecations[] = $errstr;
    }
    return true;
});

$GLOBALS['HTTP_RAW_POST_DATA'] = '
<env:Envelope xmlns:env="http://schemas.xmlsoap.org/soap/envelope/"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xmlns:xsd="http://www.w3.org/2001/XMLSchema"
    xmlns:enc="http://schemas.xmlsoap.org/soap/encoding/"
    xmlns:ns1="http://schemas.nothing.com"
>
  <env:Body>
    <dotest>
      <book xsi:type="ns1:book">
        <a xsi:type="xsd:string">Hello</a>
        <b xsi:type="xsd:string">World</b>
      </book>
    </dotest>
  </env:Body>
  <env:Header/>
</env:Envelope>';

class Book {
    public string $a = '';
    public string $b = '';
}

class TestService {
    public function dotest(Book $book): string {
        return get_class($book);
    }
}

$server = new SoapServer(__DIR__ . "/../../../ext/soap/tests/classmap.wsdl", [
    'actor'    => 'http://schema.nothing.com',
    'classmap' => ['book' => 'BOOK'],
]);
$server->setClass('TestService');
$server->handle($GLOBALS['HTTP_RAW_POST_DATA']);

echo "\n";
foreach ($deprecations as $message) {
    echo $message, "\n";
}
?>
--EXPECTF--
<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope%aSOAP-ENV:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><SOAP-ENV:Body><ns1:dotestResponse><res xsi:type="xsd:string">Book</res></ns1:dotestResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>

Using BOOK as a class name with incorrect case is deprecated, use the correct casing Book instead
