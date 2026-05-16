--TEST--
SOAP XML Schema 68: Attribute with fixed value (3)
--EXTENSIONS--
soap
xml
--FILE--
<?php
include "test_schema.inc";
$schema = <<<EOF
    <complexType name="testType">
        <attribute name="str" type="string"/>
        <attribute name="int" type="int" fixed="5"/>
    </complexType>
EOF;
test_schema($schema,'type="tns:testType"',(object)array("str"=>"str","int"=>4));
echo "ok";
?>
--EXPECTF--
Deprecated: Calling addfunction() is deprecated, use the correct casing SoapServer::addFunction() instead in %s on line %d

Fatal error: SOAP-ERROR: Encoding: Attribute 'int' has fixed value '5' (value '4' is not allowed) in %stest_schema.inc on line %d
