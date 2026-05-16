--TEST--
stream_filter_register() with wrong-case class name emits E_DEPRECATED on first use
--FILE--
<?php
class MyFilter extends php_user_filter {
    public function filter($in, $out, &$consumed, bool $closing): int {
        while ($bucket = stream_bucket_make_writeable($in)) {
            $bucket->data = strtoupper($bucket->data);
            $consumed += $bucket->datalen;
            stream_bucket_append($out, $bucket);
        }
        return PSFS_PASS_ON;
    }
}

stream_filter_register("my.upper", "MYFILTER");

$fp = fopen("php://memory", "r+");
fwrite($fp, "hello");
rewind($fp);
stream_filter_append($fp, "my.upper");
echo fread($fp, 10) . "\n";
fclose($fp);
?>
--EXPECTF--
Deprecated: Using MYFILTER as a class name with incorrect case is deprecated, use the correct casing MyFilter instead in %s on line %d
HELLO
