--TEST--
ZipArchive::registerProgressCallback() with a normal callback
--EXTENSIONS--
zip
--SKIPIF--
<?php
/* $Id$ */
if (!method_exists('ZipArchive', 'registerProgressCallback')) die('skip libzip too old');
?>
--INI--
date.timezone=UTC
--FILE--
<?php
$dirname = dirname(__FILE__) . '/';
$file = $dirname . '__tmp_oo_progress.zip';

$zip = new ZipArchive;
if (!$zip->open($file, ZIPARCHIVE::CREATE)) {
    exit('failed');
}

var_dump($zip->registerProgressCallback(0.5, function ($r) {
    // Only check start/end as intermediate is not reliable
    if ($r == 0.0) echo "start\n";
    if ($r == 1.0) echo "end\n";
}));
var_dump($zip->addFromString('foo', 'entry #1'));

var_dump($zip->close());
unlink($file);
?>
Done
--CLEAN--
<?php
$dirname = dirname(__FILE__) . '/';
$file = $dirname . '__tmp_oo_progress.zip';

@unlink($file);
?>
--EXPECT--
bool(true)
bool(true)
start
end
bool(true)
Done
