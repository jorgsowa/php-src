--TEST--
FILTER_VALIDATE_IP range flags for IPv6 addresses with embedded IPv4 notation and 64:ff9b:1::/48
--EXTENSIONS--
filter
--FILE--
<?php
// Unique-Local fc00::/7, written with a trailing IPv4 address
var_dump(filter_var('fd00::10.0.0.1', FILTER_VALIDATE_IP, FILTER_FLAG_IPV6 | FILTER_FLAG_NO_PRIV_RANGE));
var_dump(filter_var('fd00:1:2:3:4:5:10.0.0.1', FILTER_VALIDATE_IP, FILTER_FLAG_IPV6 | FILTER_FLAG_NO_PRIV_RANGE));

// Global unicast, written with a trailing IPv4 address
var_dump(filter_var('2600::8.8.8.8', FILTER_VALIDATE_IP, FILTER_FLAG_IPV6 | FILTER_FLAG_NO_RES_RANGE));
var_dump(filter_var('2600:1:2:3:4:5:8.8.8.8', FILTER_VALIDATE_IP, FILTER_FLAG_IPV6 | FILTER_FLAG_NO_RES_RANGE));
var_dump(filter_var('2600::8.8.8.8', FILTER_VALIDATE_IP, FILTER_FLAG_IPV6 | FILTER_FLAG_GLOBAL_RANGE));
var_dump(filter_var('64:ff9b::8.8.8.8', FILTER_VALIDATE_IP, FILTER_FLAG_IPV6 | FILTER_FLAG_GLOBAL_RANGE));

// Local-Use IPv4/IPv6 Translation 64:ff9b:1::/48
var_dump(filter_var('64:ff9b:1::', FILTER_VALIDATE_IP, FILTER_FLAG_IPV6 | FILTER_FLAG_GLOBAL_RANGE));
var_dump(filter_var('64:ff9b:1:ffff:ffff:ffff:ffff:ffff', FILTER_VALIDATE_IP, FILTER_FLAG_IPV6 | FILTER_FLAG_GLOBAL_RANGE));
?>
--EXPECT--
bool(false)
bool(false)
string(13) "2600::8.8.8.8"
string(22) "2600:1:2:3:4:5:8.8.8.8"
string(13) "2600::8.8.8.8"
string(16) "64:ff9b::8.8.8.8"
bool(false)
bool(false)
