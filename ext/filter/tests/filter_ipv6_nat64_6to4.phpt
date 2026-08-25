--TEST--
FILTER_FLAG_NO_PRIV_RANGE/FILTER_FLAG_NO_RES_RANGE must inspect the IPv4 address embedded in NAT64 (RFC 6052) and 6to4 (RFC 3056) IPv6 addresses
--EXTENSIONS--
filter
--FILE--
<?php
$flags = FILTER_FLAG_NO_PRIV_RANGE | FILTER_FLAG_NO_RES_RANGE;

$blocked = [
    // NAT64 well-known prefix (64:ff9b::/96) wrapping reserved/private IPv4
    '64:ff9b::7f00:1',      // 127.0.0.1
    '64:ff9b::a9fe:a9fe',   // 169.254.169.254
    '64:ff9b::a00:1',       // 10.0.0.1
    // 6to4 (2002::/16) wrapping reserved/private IPv4
    '2002:7f00:1::',        // 127.0.0.1
    '2002:a9fe:a9fe::',     // 169.254.169.254
    '2002:a00:1::',         // 10.0.0.1
];
foreach ($blocked as $ip) {
    var_dump(filter_var($ip, FILTER_VALIDATE_IP, $flags));
}

$allowed = [
    '64:ff9b::808:808',     // 8.8.8.8, genuinely public
    '2002:808:808::',       // 8.8.8.8, genuinely public
];
foreach ($allowed as $ip) {
    var_dump(filter_var($ip, FILTER_VALIDATE_IP, $flags));
}
?>
--EXPECT--
bool(false)
bool(false)
bool(false)
bool(false)
bool(false)
bool(false)
string(16) "64:ff9b::808:808"
string(14) "2002:808:808::"
