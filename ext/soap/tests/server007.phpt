--TEST--
SOAP Server 7: addfunction and getfunctions
--EXTENSIONS--
soap
--FILE--
<?php
function Add($x,$y) {
  return $x+$y;
}
function Sub($x,$y) {
  return $x-$y;
}

$server = new soapserver(null,array('uri'=>"http://testuri.org"));
$server->addfunction(array("Sub","Add"));
var_dump($server->getfunctions());
echo "ok\n";
?>
--EXPECTF--
Deprecated: Using soapserver as a class name with incorrect case is deprecated, use the correct casing SoapServer instead in %s on line %d

Deprecated: Calling addfunction() is deprecated, use the correct casing SoapServer::addFunction() instead in %s on line %d

Deprecated: Calling getfunctions() is deprecated, use the correct casing SoapServer::getFunctions() instead in %s on line %d
array(2) {
  [0]=>
  string(3) "Sub"
  [1]=>
  string(3) "Add"
}
ok
