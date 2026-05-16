--TEST--
Namespace casing check fires once for an early-bound class that extends a parent in the same namespace
--FILE--
<?php
namespace App;
class Base {}

namespace app; // same namespace as App, wrong case
class Child extends \App\Base {} // early-bindable: parent is already declared above

echo "done\n";
?>
--EXPECTF--
Deprecated: Namespace app uses incorrect casing, the canonical casing is App in %s on line 6
done
