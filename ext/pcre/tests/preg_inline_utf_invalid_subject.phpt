--TEST--
In-pattern (*UTF) validates the subject like the /u modifier
--FILE--
<?php
$subject = "a\xF4\x8F\xBF";

foreach (['/(*UTF).+/', '/(*UTF8).+/', '/(*UTF).+/u'] as $pattern) {
    echo "$pattern\n";
    var_dump(preg_match($pattern, $subject, $m), preg_last_error_msg());
    var_dump(preg_match_all($pattern, $subject, $m), preg_last_error_msg());
    var_dump(preg_replace($pattern, 'x', $subject), preg_last_error_msg());
    var_dump(preg_replace_callback($pattern, fn($m) => 'x', $subject), preg_last_error_msg());
    var_dump(preg_split($pattern, $subject), preg_last_error_msg());
    var_dump(preg_grep($pattern, [$subject]), preg_last_error_msg());
    var_dump(preg_replace_callback_array([$pattern => fn($m) => 'x'], $subject), preg_last_error_msg());
    $it = new RegexIterator(new ArrayIterator([$subject]), $pattern, RegexIterator::GET_MATCH);
    var_dump(iterator_to_array($it), preg_last_error_msg());
}

// Offset in the middle of a character
var_dump(preg_match('/(*UTF)./', "\u{E4}", $m, 0, 1), preg_last_error_msg());

var_dump(preg_match('/(*UTF)./', "\u{1F600}", $m), $m[0] === "\u{1F600}");
// Empty matches advance by whole characters
var_dump(preg_split('/(*UTF)/', "\u{E4}\u{20AC}", -1, PREG_SPLIT_NO_EMPTY));
var_dump(preg_replace('/(*UTF)/', '-', "\u{E4}\u{20AC}"));
?>
--EXPECT--
/(*UTF).+/
bool(false)
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
bool(false)
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
NULL
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
NULL
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
bool(false)
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
array(0) {
}
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
NULL
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
array(1) {
  [0]=>
  array(0) {
  }
}
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
/(*UTF8).+/
bool(false)
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
bool(false)
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
NULL
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
NULL
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
bool(false)
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
array(0) {
}
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
NULL
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
array(1) {
  [0]=>
  array(0) {
  }
}
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
/(*UTF).+/u
bool(false)
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
bool(false)
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
NULL
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
NULL
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
bool(false)
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
array(0) {
}
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
NULL
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
array(1) {
  [0]=>
  array(0) {
  }
}
string(56) "Malformed UTF-8 characters, possibly incorrectly encoded"
bool(false)
string(74) "The offset did not correspond to the beginning of a valid UTF-8 code point"
int(1)
bool(true)
array(2) {
  [0]=>
  string(2) "ä"
  [1]=>
  string(3) "€"
}
string(8) "-ä-€-"
