--TEST--
Session auto_start: next request works after request startup bailed out
--EXTENSIONS--
session
--SKIPIF--
<?php
if (!preg_match('/^session$/m', shell_exec(escapeshellarg(PHP_BINARY) . ' -n -m'))) die('skip session is not built in');
?>
--FILE--
<?php
include __DIR__ . "/../../../sapi/cli/tests/php_cli_server.inc";

$save_path = __DIR__ . '/auto_start_startup_bailout';
mkdir($save_path);
file_put_contents($save_path . '/sess_big', 'x|' . serialize(str_repeat('a', 3 * 1024 * 1024)));

php_cli_server_start('var_dump(session_status() === PHP_SESSION_ACTIVE, $_SESSION);', null, [
    '-d', 'session.save_path=' . $save_path,
    '-d', 'session.auto_start=1',
    '-d', 'memory_limit=2M',
    '-d', 'html_errors=0',
]);

echo file_get_contents('http://' . PHP_CLI_SERVER_ADDRESS, context: stream_context_create(['http' => [
    'header' => 'Cookie: PHPSESSID=big',
]]));

echo file_get_contents('http://' . PHP_CLI_SERVER_ADDRESS);
?>
--CLEAN--
<?php
$save_path = __DIR__ . '/auto_start_startup_bailout';
array_map('unlink', glob($save_path . '/*'));
rmdir($save_path);
?>
--EXPECTF--
%AFatal error: Allowed memory size of %d bytes exhausted%s(tried to allocate %d bytes)%A
bool(true)
array(0) {
}
