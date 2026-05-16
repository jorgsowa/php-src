--TEST--
Flexible heredoc lineno: ensure the compiler globals line number is correct
--FILE--
<?php

$heredoc = <<<EOT
hello world
EOT;

$heredoc = <<<'EOT'
hello world
EOT;

$heredoc = <<<EOT
 hello world
 EOT;

$heredoc = <<<'EOT'
 hello world
 EOT;

try {
	throw new exception();
} catch (Exception $e) {
	var_dump($e->getLine());
}

?>
--EXPECTF--

Deprecated: Using exception as a class name with incorrect case is deprecated, use the correct casing Exception instead in %s on line %d
int(20)
