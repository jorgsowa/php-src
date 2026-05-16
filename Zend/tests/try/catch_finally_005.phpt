--TEST--
Try catch finally (with multi-returns and exception)
--FILE--
<?php
function foo ($a) {
   try {
     throw new Exception("ex");
   } catch (PdoException $e) {
     die("error");
   } catch (Exception $e) {
     return 2;
   } finally {
     return 3;
   }
   return 1;
}

var_dump(foo("para"));
?>
--EXPECTF--

Deprecated: Using PdoException as a class name with incorrect case is deprecated, use the correct casing PDOException instead in %s on line %d
int(3)
