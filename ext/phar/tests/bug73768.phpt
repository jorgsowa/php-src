--TEST--
Phar: PHP bug #73768: Memory corruption when loading hostile phar
--EXTENSIONS--
phar
--FILE--
<?php
chdir(__DIR__);
try {
$p = Phar::LoadPhar('bug73768.phar', 'alias.phar');
echo "OK\n";
} catch(PharException $e) {
    echo $e->getMessage();
}
?>
--EXPECTF--
Deprecated: Calling LoadPhar() is deprecated, use the correct casing Phar::loadPhar() instead in %s on line %d
internal corruption of phar "%sbug73768.phar" (truncated manifest header)
