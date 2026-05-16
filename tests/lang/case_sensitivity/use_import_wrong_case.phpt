--TEST--
File-level use import with wrong-cased namespace path emits E_DEPRECATED on first use
--FILE--
<?php
namespace MyApp\Service;

class UserService {
    public function name(): string { return "UserService"; }
}

namespace Test\Wrong;

use myapp\service\UserService;

$obj = new UserService();
echo get_class($obj) . "\n";

namespace Test\AlsoWrong;

use MYAPP\SERVICE\UserService as US;

$obj2 = new US();
echo get_class($obj2) . "\n";

namespace Test\Correct;

use MyApp\Service\UserService as Correct;

$obj3 = new Correct();
echo get_class($obj3) . "\n";
?>
--EXPECTF--
Deprecated: Using myapp\service\UserService as a class name with incorrect case is deprecated, use the correct casing MyApp\Service\UserService instead in %s on line %d
MyApp\Service\UserService

Deprecated: Using MYAPP\SERVICE\UserService as a class name with incorrect case is deprecated, use the correct casing MyApp\Service\UserService instead in %s on line %d
MyApp\Service\UserService
MyApp\Service\UserService
