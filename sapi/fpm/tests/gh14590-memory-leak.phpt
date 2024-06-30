--TEST--
FPM: GH-13563 - conf boolean environment variables values
--SKIPIF--
<?php
include "skipif.inc";
FPM\Tester::skipIfRoot();
?>
--FILE--
<?php

require_once "tester.inc";

$tester = new FPM\Tester('log_buffering = \${FPM_TEST_LOG_BUF}', '');
$tester->start(envVars: [
    'FPM_TEST_LOG_BUF' => 'test',
]);
$tester->expectLogStartNotices();
$tester->terminate();
$tester->close();

?>
Done
--EXPECT--
Done
--CLEAN--
<?php
require_once "tester.inc";
FPM\Tester::clean();
?>
