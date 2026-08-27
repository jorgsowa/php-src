--TEST--
Test array_filter() function : packed array handling with different modes
--FILE--
<?php
echo "-- ARRAY_FILTER_USE_KEY with packed array --\n";
$packed = [10, 20, 30, 40, 50];
var_dump(array_filter($packed, function($key) {
    return $key > 2;
}, ARRAY_FILTER_USE_KEY));

echo "-- ARRAY_FILTER_USE_BOTH with packed array --\n";
var_dump(array_filter($packed, function($value, $key) {
    return $key > 0 && $value > 20;
}, ARRAY_FILTER_USE_BOTH));

echo "-- ARRAY_FILTER_USE_KEY with packed array (no matches) --\n";
var_dump(array_filter($packed, function($key) {
    return $key > 100;
}, ARRAY_FILTER_USE_KEY));

echo "-- ARRAY_FILTER_USE_BOTH with packed array (all match) --\n";
var_dump(array_filter($packed, function($value, $key) {
    return is_int($key) && is_int($value);
}, ARRAY_FILTER_USE_BOTH));

echo "-- ARRAY_FILTER_USE_KEY verifying keys are integers --\n";
$types = [];
array_filter($packed, function($key) use (&$types) {
    $types[] = get_debug_type($key);
    return true;
}, ARRAY_FILTER_USE_KEY);
var_dump($types);

echo "-- ARRAY_FILTER_USE_BOTH verifying keys are integers --\n";
$types = [];
array_filter($packed, function($value, $key) use (&$types) {
    $types[] = get_debug_type($key);
    return true;
}, ARRAY_FILTER_USE_BOTH);
var_dump($types);

echo "-- Packed array with gaps --\n";
$gapped = [10, 20, 30, 40, 50];
unset($gapped[2]);
var_dump(array_filter($gapped, function($key) {
    return $key >= 2;
}, ARRAY_FILTER_USE_KEY));

var_dump(array_filter($gapped, function($value, $key) {
    return $key >= 2 && $value > 30;
}, ARRAY_FILTER_USE_BOTH));

echo "-- No callback with packed array --\n";
$mixed_values = [0, 1, "", "hello", null, true, false, 42];
var_dump(array_filter($mixed_values));

?>
--EXPECT--
-- ARRAY_FILTER_USE_KEY with packed array --
array(2) {
  [3]=>
  int(40)
  [4]=>
  int(50)
}
-- ARRAY_FILTER_USE_BOTH with packed array --
array(3) {
  [2]=>
  int(30)
  [3]=>
  int(40)
  [4]=>
  int(50)
}
-- ARRAY_FILTER_USE_KEY with packed array (no matches) --
array(0) {
}
-- ARRAY_FILTER_USE_BOTH with packed array (all match) --
array(5) {
  [0]=>
  int(10)
  [1]=>
  int(20)
  [2]=>
  int(30)
  [3]=>
  int(40)
  [4]=>
  int(50)
}
-- ARRAY_FILTER_USE_KEY verifying keys are integers --
array(5) {
  [0]=>
  string(3) "int"
  [1]=>
  string(3) "int"
  [2]=>
  string(3) "int"
  [3]=>
  string(3) "int"
  [4]=>
  string(3) "int"
}
-- ARRAY_FILTER_USE_BOTH verifying keys are integers --
array(5) {
  [0]=>
  string(3) "int"
  [1]=>
  string(3) "int"
  [2]=>
  string(3) "int"
  [3]=>
  string(3) "int"
  [4]=>
  string(3) "int"
}
-- Packed array with gaps --
array(2) {
  [3]=>
  int(40)
  [4]=>
  int(50)
}
array(2) {
  [3]=>
  int(40)
  [4]=>
  int(50)
}
-- No callback with packed array --
array(4) {
  [1]=>
  int(1)
  [3]=>
  string(5) "hello"
  [5]=>
  bool(true)
  [7]=>
  int(42)
}
