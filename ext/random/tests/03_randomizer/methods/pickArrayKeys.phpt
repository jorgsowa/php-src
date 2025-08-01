--TEST--
Random: Randomizer: pickArrayKeys(): Basic functionality
--FILE--
<?php

use Random\Engine;
use Random\Engine\Mt19937;
use Random\Engine\PcgOneseq128XslRr64;
use Random\Engine\Secure;
use Random\Engine\Test\TestShaEngine;
use Random\Engine\Xoshiro256StarStar;
use Random\Randomizer;

require __DIR__ . "/../../engines.inc";

$engines = [];
$engines[] = new Mt19937(null, MT_RAND_MT19937);
$engines[] = new Mt19937(null, MT_RAND_PHP);
$engines[] = new PcgOneseq128XslRr64();
$engines[] = new Xoshiro256StarStar();
$engines[] = new Secure();
$engines[] = new TestShaEngine();
$iterations = getenv("SKIP_SLOW_TESTS") ? 10 : 100;

$array1 = []; // list
$array2 = []; // associative array with only strings
$array3 = []; // mixed key array
for ($i = 0; $i < 500; $i++) {
    $string = sha1((string)$i);

    $array1[] = $i;
    $array2[$string] = $i;
    $array3[$string] = $i;
    $array3[$i] = $string;
}

foreach ($engines as $engine) {
    echo $engine::class, PHP_EOL;

    $randomizer = new Randomizer($engine);

    for ($i = 1; $i < $iterations; $i++) {
        $result = $randomizer->pickArrayKeys($array1, $i);

        if (array_unique($result) !== $result) {
            die("failure: duplicates returned at {$i} for array1");
        }

        if (array_diff($result, array_keys($array1)) !== []) {
            die("failure: non-keys returned at {$i} for array1");
        }

        $result = $randomizer->pickArrayKeys($array2, $i);

        if (array_unique($result) !== $result) {
            die("failure: duplicates returned at {$i} for array2");
        }

        if (array_diff($result, array_keys($array2)) !== []) {
            die("failure: non-keys returned at {$i} for array2");
        }

        $result = $randomizer->pickArrayKeys($array3, $i);

        if (array_unique($result) !== $result) {
            die("failure: duplicates returned at {$i} for array3");
        }

        if (array_diff($result, array_keys($array3)) !== []) {
            die("failure: non-keys returned at {$i} for array3");
        }
    }
}

die('success');

?>
--EXPECTF--
Deprecated: Constant MT_RAND_PHP is deprecated since 8.3, as it uses a biased non-standard variant of Mt19937 in %s on line %d

Deprecated: The MT_RAND_PHP variant of Mt19937 is deprecated in %s on line %d
Random\Engine\Mt19937
Random\Engine\Mt19937
Random\Engine\PcgOneseq128XslRr64
Random\Engine\Xoshiro256StarStar
Random\Engine\Secure
Random\Engine\Test\TestShaEngine
success
