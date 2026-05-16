--TEST--
namespace declaration with inconsistent casing emits E_DEPRECATED
--FILE--
<?php
namespace MyApp\Service {
    class UserService {}
}

// Same namespace, wrong case — canonical casing set by UserService above
namespace MYAPP\Service {
    class OtherService {}
}

// Another wrong case
namespace myapp\service {
    class ThirdService {}
}

// Correct case — no warning
namespace MyApp\Service {
    class AnotherService {}
}
?>
--EXPECTF--
Deprecated: Namespace MYAPP\Service uses incorrect casing, the canonical casing is MyApp\Service in %s on line %d

Deprecated: Namespace myapp\service uses incorrect casing, the canonical casing is MyApp\Service in %s on line %d
