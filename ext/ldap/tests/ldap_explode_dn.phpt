--TEST--
ldap_explode_dn() test
--EXTENSIONS--
ldap
--FILE--
<?php

/* Explode with attributes */
var_dump(ldap_explode_dn("cn=bob,dc=example,dc=com", 0));

/* Explode with attributes */
var_dump(ldap_explode_dn("cn=bob,ou=users,dc=example,dc=com", 0));

/* Explode without attributes */
var_dump(ldap_explode_dn("cn=bob,dc=example,dc=com", 1));

/* Explode without attributes */
var_dump(ldap_explode_dn("cn=bob,ou=users,dc=example,dc=com", 1));

/* Explode with attributes and < > characters */
var_dump(ldap_explode_dn("cn=<bob>,dc=example,dc=com", 0));

/* Explode without attributes and < > characters */
var_dump(ldap_explode_dn("cn=<bob>,dc=example,dc=com", 1));

/* Bad DN value with attributes */
var_dump(ldap_explode_dn("bob,dc=example,dc=com", 0));

/* Bad DN value without attributes */
var_dump(ldap_explode_dn("bob,dc=example,dc=com", 1));

echo "Done\n";

?>
--EXPECT--
array(4) {
  [0]=>
  string(6) "cn=bob"
  [1]=>
  string(10) "dc=example"
  [2]=>
  string(6) "dc=com"
  ["count"]=>
  int(3)
}
array(5) {
  [0]=>
  string(6) "cn=bob"
  [1]=>
  string(8) "ou=users"
  [2]=>
  string(10) "dc=example"
  [3]=>
  string(6) "dc=com"
  ["count"]=>
  int(4)
}
array(4) {
  [0]=>
  string(3) "bob"
  [1]=>
  string(7) "example"
  [2]=>
  string(3) "com"
  ["count"]=>
  int(3)
}
array(5) {
  [0]=>
  string(3) "bob"
  [1]=>
  string(5) "users"
  [2]=>
  string(7) "example"
  [3]=>
  string(3) "com"
  ["count"]=>
  int(4)
}
bool(false)
bool(false)
bool(false)
bool(false)
Done
