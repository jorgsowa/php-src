--TEST--
SOAP Server 8: setclass and getfunctions
--EXTENSIONS--
soap
--FILE--
<?php
class Foo {

  function __construct() {
  }

  function test() {
    return $this->str;
  }
}

$server = new soapserver(null,array('uri'=>"http://testuri.org"));
$server->setclass("Foo");
var_dump($server->getfunctions());
echo "ok\n";
?>
--EXPECTF--
Deprecated: Using soapserver as a class name with incorrect case is deprecated, use the correct casing SoapServer instead in %s on line %d

Deprecated: Calling setclass() is deprecated, use the correct casing SoapServer::setClass() instead in %s on line %d

Deprecated: Calling getfunctions() is deprecated, use the correct casing SoapServer::getFunctions() instead in %s on line %d
array(2) {
  [0]=>
  string(11) "__construct"
  [1]=>
  string(4) "test"
}
ok
