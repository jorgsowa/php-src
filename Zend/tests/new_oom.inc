<?php

/* get_declared_classes() can return non-canonical casing for some aliased
 * internal classes (e.g. DOM legacy aliases), which makes the ReflectionClass
 * lookup below emit E_DEPRECATED about the casing. That is unrelated to this
 * OOM test, so deprecations are silenced to keep the output deterministic. */
error_reporting(error_reporting() & ~E_DEPRECATED);

$mb_used = (int) ceil(memory_get_usage() / (1024 ** 2));
ini_set('memory_limit', ($mb_used + 1) . 'M');

$class = $argv[1];
$objects = [];

try {
    while (true) {
        $rc = new ReflectionClass($class);
        $objects[] = $rc->newInstanceWithoutConstructor();
    }
} catch (Throwable) {
}
