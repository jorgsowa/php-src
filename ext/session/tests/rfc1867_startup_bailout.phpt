--TEST--
Session upload progress: next request works after request startup bailed out
--EXTENSIONS--
session
--SKIPIF--
<?php
if (!preg_match('/^session$/m', shell_exec(escapeshellarg(PHP_BINARY) . ' -n -m'))) die('skip session is not built in');
?>
--FILE--
<?php
include __DIR__ . "/../../../sapi/cli/tests/php_cli_server.inc";

$save_path = __DIR__ . '/rfc1867_startup_bailout';
mkdir($save_path);

php_cli_server_start('session_start(); var_dump(session_status() === PHP_SESSION_ACTIVE, $_SESSION);', null, [
    '-d', 'session.save_path=' . $save_path,
    '-d', 'memory_limit=2M',
    '-d', 'html_errors=0',
]);

$body = "--b\r\nContent-Disposition: form-data; name=\"PHP_SESSION_UPLOAD_PROGRESS\"\r\n\r\nx\r\n"
    . "--b\r\nContent-Disposition: form-data; name=\"file\"; filename=\"file.txt\"\r\n\r\nx\r\n"
    . "--b\r\nContent-Disposition: form-data; name=\"big\"\r\n\r\n" . str_repeat('a', 3 * 1024 * 1024) . "\r\n"
    . "--b--\r\n";
echo file_get_contents('http://' . PHP_CLI_SERVER_ADDRESS, context: stream_context_create(['http' => [
    'method' => 'POST',
    'header' => "Content-Type: multipart/form-data; boundary=b\r\nCookie: PHPSESSID=uploadprogress",
    'content' => $body,
]]));

echo file_get_contents('http://' . PHP_CLI_SERVER_ADDRESS);
?>
--CLEAN--
<?php
$save_path = __DIR__ . '/rfc1867_startup_bailout';
array_map('unlink', glob($save_path . '/*'));
rmdir($save_path);
?>
--EXPECTF--
%AFatal error: Allowed memory size of %d bytes exhausted%s(tried to allocate %d bytes)%A
bool(true)
array(0) {
}
