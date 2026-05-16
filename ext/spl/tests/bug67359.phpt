--TEST--
Bug #67359 (Segfault in recursiveDirectoryIterator)
--FILE--
<?php
try
{
    $rdi = new recursiveDirectoryIterator(__DIR__,  FilesystemIterator::SKIP_DOTS | FilesystemIterator::UNIX_PATHS);
    $it = new recursiveIteratorIterator( $rdi );
    $it->seek(1);
    while( $it->valid())
    {
        if( $it->isFile() )
        {
            $it->current();
        }

        $it->next();
    }

    $it->current();
}
catch(Exception $e)
{
}
echo "okey"
?>
--EXPECTF--
Deprecated: Using recursiveDirectoryIterator as a class name with incorrect case is deprecated, use the correct casing RecursiveDirectoryIterator instead in %s on line %d

Deprecated: Using recursiveIteratorIterator as a class name with incorrect case is deprecated, use the correct casing RecursiveIteratorIterator instead in %s on line %d
okey
