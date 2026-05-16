====== PHP RFC: Case-sensitive PHP ======
  * Version: 1.0
  * Date: 2026-06-01
  * Author: Jorg Sowa <jorg.sowa@gmail.com>
  * Status: Draft
  * Implementation: https://github.com/php/php-src/pull/22260
  * Discussion thread: TBD
  * Voting thread: TBD

===== Introduction =====

PHP has always treated function, method, and class names as case-insensitive. That was a pragmatic early design decision, but it now creates inconsistency without much benefit.

Today, all of these are valid PHP:

<code php>
namespace MyApp\Service;

class UserService {}
function myHelper(): void {}

// Functions — all three resolve to the same strlen()
strlen("hello");    // canonical
STRLEN("hello");    // works
StrLen("hello");    // also works

// Classes — all three resolve to the same UserService
new UserService();  // canonical
new USERSERVICE();  // works
new userservice();  // also works

// Namespaces — wrong casing on the namespace prefix also resolves
new \myapp\service\UserService();  // works despite wrong namespace casing
</code>

This RFC proposes emitting ''E_DEPRECATED'' warnings in PHP 8.6 when functions, methods, or classes are referenced with incorrect casing. The goal is to give developers and tools time to adapt before enforcement becomes fatal in the next major version.

Case-insensitivity only applies to ASCII identifiers. The engine's ''zend_tolower_map'' lookup table only lowercases bytes ''0x41''–''0x5A'' (A–Z); bytes above ''0x7F'' pass through unchanged, so non-ASCII identifiers are already case-sensitive today:

<code php>
class Ñoño {}
new Ñoño();      // works
new ñoño();      // Fatal error — non-ASCII, already case-sensitive
STRLEN("hello"); // works — ASCII, this RFC deprecates this
</code>

The deprecation warnings introduced by this RFC cover ASCII identifiers only, consistent with the existing engine behavior. Unicode case folding is out of scope.

==== Case sensitivity in PHP today ====

PHP is already partially case-sensitive. This RFC addresses the remaining inconsistencies:

**Case-insensitive (as of PHP 8.5):**

^ Identifier ^ Note ^
| Function names (user-defined and built-in) | deprecated by this RFC |
| Method names | deprecated by this RFC |
| Class, interface, and trait names | deprecated by this RFC |
| Magic method names (''%%__construct%%'', ''%%__toString%%'') | deprecated by this RFC |
| Namespace names in class references and ''use'' imports | deprecated by this RFC |
| ''namespace'' declarations (inconsistent casing across files) | deprecated by this RFC |
| Keywords (''if'', ''else'', ''for'', ''while'', ''class'', ''function'', ''use'', ''match'', ''fn'', ''readonly'', ...) | |
| Built-in type names in type declarations (''int'', ''string'', ''bool'', ''void'', ...) | |
| Special class references (''self'', ''parent'', ''static'') | |
| ''true'', ''false'', ''null'' | |
| Magic constants (''%%__CLASS__%%'', ''%%__FUNCTION__%%'', ''%%__METHOD__%%'', ''%%__LINE__%%'', ''%%__FILE__%%'', ''%%__DIR__%%'', ''%%__NAMESPACE__%%'', ''%%__TRAIT__%%'', ''%%__PROPERTY__%%'') | |

**Case-sensitive (already enforced):**

^ Identifier ^ Example ^
| Variables | ''$foo'' != ''$Foo'' |
| Constants | ''FOO'' != ''foo'', fatal error on mismatch |
| Object properties | ''$obj->name'' != ''$obj->Name'' |
| Array keys | ''"key"'' != ''"Key"'' |
| Enum cases | ''Color::Red'' != ''Color::red'', fatal error |
| Goto labels | ''myLabel'' != ''MYLABEL'' |

After this RFC is fully enforced in the next major version, all user-defined identifiers in PHP will be case-sensitive. The remaining case-insensitive constructs will all be language-defined: keywords (''if'', ''while'', ''match'', ''class'', ''function'', etc.), built-in type names (''int'', ''string'', etc.), special class references (''self'', ''parent'', ''static''), the literals ''true'', ''false'', ''null'', and magic constants (''%%__CLASS__%%'', ''%%__FUNCTION__%%'', etc.).

==== Language comparison ====

PHP is one of the few remaining modern languages that does not enforce case sensitivity for user-defined identifiers.

^ Language ^ Case-sensitive? ^ Notes ^
| Python | Yes | |
| JavaScript | Yes | |
| TypeScript | Yes | |
| Ruby | Yes | Capitalized identifiers are constants |
| Go | Yes | Case determines visibility: uppercase = exported (public), lowercase = unexported (private) |
| Rust | Yes | Compiler warns on convention violations (''snake_case'' vs ''CamelCase'') |
| Java | Yes | |
| C# | Yes | Guidelines discourage names differing only in case (CLR interop) |
| Swift | Yes | |
| Kotlin | Yes | |
| Perl | Yes | |
| Lua | Yes | |
| Visual Basic / VBA | No | Classic VB (1991) and VB.NET (2002) |
| SQL | No | Keywords and identifiers are case-insensitive; string literals are case-sensitive |
| Pascal / Delphi | No | Case-insensitive since the 1970s |
| COBOL | No | Keywords and identifiers only; string literals are case-sensitive |
| PowerShell | No | 2006; cmdlets, functions, and variable names are case-insensitive |
| PHP | No (partial) | See "Case sensitivity in PHP today" |

Among general-purpose languages in active use today, case sensitivity is the norm. Go is the most instructive: it did not just enforce consistent casing, it made the first letter semantically meaningful (access control). PHP cannot reasonably do that at this stage, but removing the implicit lowercasing brings it in line with the mainstream.

The case-insensitive languages in the table split into two groups. Pascal, Delphi, COBOL, and classic Visual Basic predate PHP 3 (1997) and established the convention PHP inherited. PowerShell (2006) is a shell, and its case-insensitive tradition carried over from earlier scripting environments. VB.NET (2002) postdates PHP 3 but is the direct successor of classic Visual Basic and retains the behavior for backward compatibility. None of these languages are the general-purpose, server-side peers PHP competes with today.

==== PSR-4 autoloading and filesystem portability ====

PHP's case-insensitive class lookup interacts badly with [[https://www.php-fig.org/psr/psr-4/|PSR-4 autoloading]] and filesystem case sensitivity in a way that hides bugs during development and surfaces them in production.

PSR-4 maps a fully-qualified class name directly to a file path: ''App\Service\UserService'' → ''app/Service/UserService.php''. The autoloader constructs that path from the class name as written at the call site, then opens the file. Whether that file open succeeds depends entirely on the filesystem.

^ Environment ^ Filesystem ^ ''new app\service\USERSERVICE()'' ^
| Linux (production) | ext4, btrfs (case-sensitive) | Autoloader fails — file not found |
| macOS (developer) | HFS+ / APFS case-insensitive (default) | Autoloader succeeds — file found |
| Windows (developer) | NTFS (case-insensitive) | Autoloader succeeds — file found |

The result is a class of bugs that passes silently on developer machines and breaks only on Linux servers. A wrong-cased ''new APP\SERVICE\USERSERVICE()'' works fine locally, passes CI if CI also runs on macOS or Windows, and then throws a fatal error on the production host.

There is a second subtlety: if the class is already in PHP's class registry (loaded earlier in the same request via a correctly-cased reference), PHP's case-insensitive lookup resolves the wrong-cased reference without ever calling the autoloader. That makes the bug intermittent: it disappears when the class happens to be loaded first by another code path, and reappears when the execution order changes.

This RFC's deprecation warning fires at the PHP engine level regardless of whether the autoloader was involved, which catches both cases.

===== Proposal =====

Emit ''E_DEPRECATED'' when any of the following identifiers are referenced with incorrect casing:

**Calls** (2.1–2.3)
  * Function calls — user-defined and built-in (section 2.1)
  * Method calls — instance methods, including dynamic calls (section 2.2)
  * Static method calls (section 2.3)

**Language constructs** (2.4–2.14)
  * Class instantiation via ''new'' (section 2.4)
  * Namespace segments in class references (section 2.5)
  * ''instanceof'' checks (section 2.6)
  * Type declarations — parameter types, return types, and property types (section 2.7)
  * Generator return types — ''Traversable'', ''Iterator'', ''Generator'' written with wrong case (section 2.8)
  * Class constant and enum case access — ''ClassName::CONST'', ''EnumName::Case'' (section 2.9)
  * Static property access — ''ClassName::$prop'' (section 2.10)
  * Catch clauses (section 2.11)
  * ''extends'' — wrong-cased parent class name (section 2.12)
  * ''implements'' — wrong-cased interface name (section 2.13)
  * ''use'' — wrong-cased trait name in a class body (section 2.14)

**Callables and dynamic dispatch** (2.15–2.16)
  * Callable class names — array callables ''["ClassName", "method"]'' and string callables ''"ClassName::method"'' (section 2.15)
  * ''Closure::bind()'' and ''bindTo()'' — wrong-cased scope class name (section 2.16)

**Class and function introspection** (2.17–2.23)
  * ''class_exists()'', ''interface_exists()'', ''trait_exists()'', ''enum_exists()'' (section 2.17)
  * ''function_exists()'' — wrong-cased function name string (section 2.18)
  * ''class_alias()'' — wrong-cased original class name (section 2.19)
  * ''is_a()'' and ''is_subclass_of()'' — wrong-cased class name in both the subject string and the class name argument (section 2.20)
  * ''class_parents()'', ''class_implements()'', ''class_uses()'' — wrong-cased class name (section 2.21)
  * ''property_exists()'' — wrong-cased class name string (section 2.22)
  * ''method_exists()'' — wrong-cased class name string and wrong-cased method name (section 2.23)

**Reflection API** (2.24–2.35)
  * ''ReflectionClass'' constructor — wrong-cased class name argument (section 2.24)
  * ''ReflectionAttribute::newInstance()'' — wrong-cased attribute class name (section 2.25)
  * ''ReflectionFunction'' constructor — wrong-cased function name (section 2.26)
  * ''ReflectionMethod'' constructor — wrong-cased class name (section 2.27)
  * ''ReflectionProperty'' constructor — wrong-cased class name (section 2.28)
  * ''ReflectionClassConstant'' constructor — wrong-cased class name (section 2.29)
  * ''ReflectionClass::isSubclassOf()'' — wrong-cased class name (section 2.30)
  * ''ReflectionClass::implementsInterface()'' — wrong-cased interface name (section 2.31)
  * ''ReflectionClass::getAttributes()'' with ''IS_INSTANCEOF'' — wrong-cased class name (section 2.32)
  * ''ReflectionParameter'' constructor — wrong-cased class name in array callable (section 2.33)
  * ''ReflectionProperty::isReadable()'' and ''isWritable()'' — wrong-cased scope class name (section 2.34)
  * ''ReflectionClass::getProperty()'' with ''"ClassName::$prop"'' syntax — wrong-cased class name prefix (section 2.35)

**Declarations** (2.36–2.38)
  * Magic method declarations — declaring ''%%__CONSTRUCT%%'', ''%%__toString%%'', ''%%__sleep%%'', etc. with wrong case (section 2.36)
  * File-level ''use'' imports — wrong-cased class or namespace path in ''use'', ''use function'', and ''use const'' declarations (section 2.37)
  * ''namespace'' declarations — inconsistent namespace casing across files in the same namespace (section 2.38)

**Serialization** (2.39–2.40)
  * ''unserialize()'' — wrong-cased class or enum name in serialized object/enum data (section 2.39)
  * ''ArrayObject::%%__unserialize%%()'' — wrong-cased iterator class name in serialized data (section 2.40)

**Extensions and SPL** (2.41–2.46)
  * ''SoapServer''/''SoapClient'' classmap — wrong-cased PHP class name in ''classmap'' option (section 2.41)
  * ''ArrayObject::setIteratorClass()'' — wrong-cased iterator class name (section 2.42)
  * ''IteratorIterator'' and ''RecursiveIteratorIterator'' — wrong-cased inner iterator class cast (section 2.43)
  * ''stream_filter_register()'' — wrong-cased filter class name (section 2.44)
  * ''PDO::ATTR_STATEMENT_CLASS'' — wrong-cased statement class name (section 2.45)
  * ''PDOStatement::setFetchMode(PDO::FETCH_CLASS)'' — wrong-cased fetch class name (section 2.46)

==== 2.1 Function calls ====

User-defined and built-in function names must match the declaration casing. Direct calls, dynamic calls (''$fn()''), ''call_user_func()'', and ''function_exists()'' are all covered.

<code php>
function myFunction(): int { return 42; }

// Correct — no warning
echo strlen("hello");
echo myFunction();

// Incorrect — E_DEPRECATED
echo STRLEN("hello");    // Deprecated: Calling STRLEN() is deprecated,
                         //             use the correct casing strlen() instead
echo MYFUNCTION();       // Deprecated: Calling MYFUNCTION() is deprecated,
                         //             use the correct casing myFunction() instead
</code>

==== 2.2 Method calls ====

Instance method names must match their declaration casing. Dynamic method calls (''$obj->$name()'') are also checked.

<code php>
class MyClass {
    public function myMethod(): string { return "called"; }
}

$obj = new MyClass();

// Correct — no warning
echo $obj->myMethod();

// Incorrect — E_DEPRECATED
echo $obj->MyMethod();   // Deprecated: Calling MyMethod() is deprecated,
                         //             use the correct casing MyClass::myMethod() instead
</code>

==== 2.3 Static method calls ====

<code php>
class MathHelper {
    public static function square(int $n): int { return $n ** 2; }
}

// Correct — no warning
echo MathHelper::square(4);

// Incorrect — E_DEPRECATED
echo MathHelper::SQUARE(4); // Deprecated: Calling SQUARE() is deprecated,
                             //             use the correct casing MathHelper::square() instead
</code>

==== 2.4 Class instantiation ====

Class names in ''new'' expressions must match the declaration casing.

<code php>
class ProductService {}

// Correct — no warning
$obj = new ProductService();

// Incorrect — E_DEPRECATED
$obj = new PRODUCTSERVICE(); // Deprecated: Using PRODUCTSERVICE as a class name
                              //             with incorrect case is deprecated,
                              //             use the correct casing ProductService instead
</code>

==== 2.5 Namespace segments in class references ====

Every segment of a fully-qualified class name must match its declaration, including the namespace prefix. This applies to both literal fully-qualified names and names resolved from ''use'' imports (see section 2.37).

<code php>
namespace MyApp\Service;

class UserService {}

// Correct — no warning
$s = new \MyApp\Service\UserService();

// Incorrect — E_DEPRECATED
$s = new \myapp\service\UserService(); // Deprecated: Using myapp\service\UserService
                                       //             as a class name with incorrect case...
</code>

==== 2.6 ''instanceof'' checks ====

<code php>
class MyException extends \Exception {}

$ex = new MyException();

// Correct — no warning
var_dump($ex instanceof MyException);

// Incorrect — E_DEPRECATED
var_dump($ex instanceof myexception); // Deprecated: Using myexception as a class name
                                      //             with incorrect case is deprecated,
                                      //             use the correct casing MyException instead
</code>

==== 2.7 Type declarations ====

Parameter types, return types, and property types are checked when the function is first called or the property is first assigned.

<code php>
class MyService {}

// Correct — no warning
function handle(MyService $svc): MyService { return $svc; }

// Incorrect — E_DEPRECATED (fires on first call)
function handleWrong(MYSERVICE $svc): MYSERVICE { return $svc; }
// Deprecated: Using MYSERVICE as a class name with incorrect case is deprecated,
//             use the correct casing MyService instead

// Property types are also covered — E_DEPRECATED on first assignment
class Container {
    public MYSERVICE $service;
}
</code>

==== 2.8 Generator return types ====

A function is a generator when its body contains ''yield''. Its declared return type may be one of the built-in iterator interfaces: ''Traversable'', ''Iterator'', or ''Generator''. The generator-compatibility check runs at compile time, so wrong casing on these specific names is caught at compile time rather than on the first call.

<code php>
// Incorrect — E_DEPRECATED at compile time
function gen(): ITERATOR {
    yield 1;
}
// Deprecated: Using ITERATOR as a class name with incorrect case is deprecated,
//             use the correct casing Iterator instead
</code>

This check fires in ''zend_mark_function_as_generator()'' during compilation. Under opcache it fires once at warmup and is never re-triggered from cached bytecode. It is emitted exactly once per function (the generator-compatibility test also runs once per ''yield'' expression, but only the function-close pass requests the casing check). Other class-typed return types are covered by the general type-declaration check (section 2.7), which fires at runtime on first call.

==== 2.9 Class constant and enum case access ====

The class name prefix in a class constant access must match the declaration casing. Because enum cases are class constants, the same applies to enum case access.

<code php>
class Status {
    const ACTIVE = 1;
}

enum Suit: string {
    case Hearts = 'H';
}

// Correct — no warning
echo Status::ACTIVE;
echo Suit::Hearts->value;

// Incorrect — E_DEPRECATED
echo STATUS::ACTIVE; // Deprecated: Using STATUS as a class name with incorrect case
                     //             is deprecated, use the correct casing Status instead

echo SUIT::Hearts->value; // Deprecated: Using SUIT as a class name with incorrect case
                          //             is deprecated, use the correct casing Suit instead
</code>

Only the class name is checked, not the constant or case name. Constants and enum case names are already case-sensitive in PHP, where a mismatch is a fatal error rather than a deprecation. When the class name is known at compile time the check runs during compilation (in ''zend_compile_class_const()''); otherwise it runs at runtime on the ''ZEND_FETCH_CLASS_CONSTANT'' opcode.

==== ''%%::class%%'' name resolution ====

''MyClass::class'' folds to a string at compile time with no class lookup, so a wrong-cased ''myclass::class'' historically produced ''"myclass"'' with no warning. When the class is already declared at compile time, the casing is now checked against the declaration, the same way class-constant access is (section 2.9).

<code php>
class UserService {}

// Correct — no warning
$name = UserService::class;   // "UserService"

// Incorrect — E_DEPRECATED at compile time
$name = userservice::class;
// Deprecated: Using userservice as a class name with incorrect case is deprecated,
//             use the correct casing UserService instead
</code>

The check runs in ''zend_try_compile_const_expr_resolve_class_name()'': after the name resolves to its fully-qualified form, the compiler looks it up in ''CG(class_table)'' and calls ''zend_check_class_name_case()'' when the class is present. If the class is not known at that point (the usual case for autoloaded classes), the name folds as before and the ''%%::class%%'' site stays silent. The wrong-cased string is then caught by whatever consumes it (''new $name()'', ''class_exists($name)'', the Reflection constructors, and the rest of section 2), since those do a real lookup. ''self::class'', ''parent::class'', ''static::class'', and ''$object::class'' are unaffected; their casing comes from the engine.

==== 2.10 Static property access ====

The class name prefix in a static property access (read, write, or ''unset'') must match the declaration casing.

<code php>
class Counter {
    public static int $value = 0;
}

// Correct — no warning
Counter::$value++;

// Incorrect — E_DEPRECATED
COUNTER::$value++;
// Deprecated: Using COUNTER as a class name with incorrect case is deprecated,
//             use the correct casing Counter instead
</code>

The property name itself is already case-sensitive and is not affected. The check runs at runtime when the static property address is resolved.

==== 2.11 Catch clauses ====

<code php>
class DatabaseException extends \RuntimeException {}

// Correct — no warning
try {
    throw new DatabaseException();
} catch (DatabaseException $e) { /* ... */ }

// Incorrect — E_DEPRECATED
try {
    throw new DatabaseException();
} catch (databaseexception $e) { // Deprecated: Using databaseexception as a class name
    /* ... */                     //             with incorrect case is deprecated,
}                                 //             use the correct casing DatabaseException instead
</code>

==== 2.12 ''extends'' ====

The parent class name in an ''extends'' clause must match its declaration casing.

<code php>
class BaseRepository {}

// Incorrect — E_DEPRECATED
class UserRepository extends BASEREPOSITORY {}
// Deprecated: Using BASEREPOSITORY as a class name with incorrect case is deprecated,
//             use the correct casing BaseRepository instead
</code>

==== 2.13 ''implements'' ====

Interface names in ''implements'' clauses must match their declaration casing.

<code php>
interface JsonExportable {}

// Incorrect — E_DEPRECATED
class User implements JSONEXPORTABLE {}
// Deprecated: Using JSONEXPORTABLE as a class name with incorrect case is deprecated,
//             use the correct casing JsonExportable instead
</code>

==== 2.14 ''use'' (trait) ====

Trait names in ''use'' statements inside a class body must match their declaration casing.

<code php>
trait Timestampable {
    public function touch(): void {}
}

// Incorrect — E_DEPRECATED
class Post {
    use TIMESTAMPABLE;
    // Deprecated: Using TIMESTAMPABLE as a class name with incorrect case is deprecated,
    //             use the correct casing Timestampable instead
}
</code>

==== 2.15 Callable class names ====

Class names used as part of callables are checked, in both the array form and the ''"Class::method"'' string form.

<code php>
class MyService {
    public static function process(): string { return "ok"; }
}

// Incorrect array callable — E_DEPRECATED
call_user_func(["myservice", "process"]);
// Deprecated: Using myservice as a class name with incorrect case is deprecated,
//             use the correct casing MyService instead

// Incorrect string callable — E_DEPRECATED
call_user_func("MYSERVICE::process");
</code>

==== 2.16 ''Closure::bind()'' and ''bindTo()'' ====

The scope class name passed to ''Closure::bind()'' or ''bindTo()'' as a string must match the declaration casing.

<code php>
class MyScope {
    private int $x = 1;
}

$f = function() { return $this->x; };

// Incorrect — E_DEPRECATED
$bound = Closure::bind($f, new MyScope, "myscope");
// Deprecated: Using myscope as a class name with incorrect case is deprecated,
//             use the correct casing MyScope instead
</code>

==== 2.17 ''class_exists()'' family ====

''class_exists()'', ''interface_exists()'', ''trait_exists()'', and ''enum_exists()'' validate the class name casing.

<code php>
class UserRepository {}

// Correct — no warning
class_exists('UserRepository');

// Incorrect — E_DEPRECATED
class_exists('userrepository'); // Deprecated: Using userrepository as a class name
                                //             with incorrect case is deprecated,
                                //             use the correct casing UserRepository instead
</code>

==== 2.18 ''function_exists()'' ====

When a wrong-cased function name string is passed to ''function_exists()'', the casing must match the declaration. The function returns its normal result; the deprecation fires alongside it.

<code php>
function myHelper(): void {}

// Correct — no warning
function_exists('myHelper');
function_exists('strlen');

// Incorrect — E_DEPRECATED
function_exists('MYHELPER'); // Deprecated: Calling MYHELPER() is deprecated,
                             //             use the correct casing myHelper() instead
</code>

==== 2.19 ''class_alias()'' ====

The original class name argument to ''class_alias()'' must match the declaration casing.

<code php>
class UserService {}

// Incorrect — E_DEPRECATED
class_alias("userservice", "US");
// Deprecated: Using userservice as a class name with incorrect case is deprecated,
//             use the correct casing UserService instead
</code>

==== 2.20 ''is_a()'' and ''is_subclass_of()'' ====

When a class name string is passed as the class name argument (second argument) to ''is_a()'' or ''is_subclass_of()'', the casing must match the declaration. The casing of the subject (first argument) is checked too when it is a string rather than an object, which is allowed when the third argument is set.

<code php>
class BaseModel {}
class User extends BaseModel {}

$user = new User();

// Correct — no warning
var_dump(is_a($user, 'BaseModel'));
var_dump(is_subclass_of($user, 'BaseModel'));

// Incorrect class name argument — E_DEPRECATED
var_dump(is_a($user, 'basemodel'));
// Deprecated: Using basemodel as a class name with incorrect case is deprecated,
//             use the correct casing BaseModel instead

// Incorrect subject string (third argument allows string subjects) — E_DEPRECATED
var_dump(is_a('USER', 'BaseModel', true));
// Deprecated: Using USER as a class name with incorrect case is deprecated,
//             use the correct casing User instead
</code>

==== 2.21 ''class_parents()'', ''class_implements()'', ''class_uses()'' ====

The class name string argument to these SPL functions must match the declaration casing.

<code php>
class Base {}
class Child extends Base {}

// Incorrect — E_DEPRECATED
class_parents("CHILD");
// Deprecated: Using CHILD as a class name with incorrect case is deprecated,
//             use the correct casing Child instead
</code>

==== 2.22 ''property_exists()'' ====

When the first argument is a class name string, the casing must match the declaration.

<code php>
class MyClass {
    public int $value = 0;
}

// Incorrect — E_DEPRECATED
property_exists("MYCLASS", "value");
// Deprecated: Using MYCLASS as a class name with incorrect case is deprecated,
//             use the correct casing MyClass instead
</code>

==== 2.23 ''method_exists()'' ====

When the first argument is a class name string, its casing must match the declaration. The method name (second argument) is checked too, so either one can emit a deprecation.

<code php>
class MyService {
    public function handle(): void {}
}

// Incorrect class name — E_DEPRECATED
method_exists("MYSERVICE", "handle");
// Deprecated: Using MYSERVICE as a class name with incorrect case is deprecated,
//             use the correct casing MyService instead

// Incorrect method name — E_DEPRECATED
method_exists("MyService", "HANDLE");
// Deprecated: Calling HANDLE() is deprecated,
//             use the correct casing MyService::handle() instead
</code>

==== 2.24 ''ReflectionClass'' constructor ====

Instantiating ''ReflectionClass'' with a wrong-cased class name string emits a deprecation.

<code php>
class MyModel {}

// Incorrect — E_DEPRECATED
$rc = new ReflectionClass("mymodel");
// Deprecated: Using mymodel as a class name with incorrect case is deprecated,
//             use the correct casing MyModel instead
</code>

==== 2.25 ''ReflectionAttribute::newInstance()'' ====

When an attribute is applied using a wrong-cased name, calling ''newInstance()'' on the resulting ''ReflectionAttribute'' emits a deprecation.

<code php>
#[Attribute]
class MyAttr {}

// Incorrect attribute application — E_DEPRECATED when newInstance() is called
#[MYATTR]
class Foo {}

$attrs = (new ReflectionClass(Foo::class))->getAttributes();
$attrs[0]->newInstance();
// Deprecated: Using MYATTR as a class name with incorrect case is deprecated,
//             use the correct casing MyAttr instead
</code>

==== 2.26 ''ReflectionFunction'' constructor ====

Instantiating ''ReflectionFunction'' with a wrong-cased function name emits a deprecation.

<code php>
function myFunc(): int { return 42; }

// Incorrect — E_DEPRECATED
$rf = new ReflectionFunction("MYFUNC");
// Deprecated: Calling MYFUNC() is deprecated, use the correct casing myFunc() instead
</code>

==== 2.27 ''ReflectionMethod'' constructor ====

Instantiating ''ReflectionMethod'' with a wrong-cased class name emits a deprecation.

<code php>
class MyService {
    public function handle(): void {}
}

// Incorrect — E_DEPRECATED
$rm = new ReflectionMethod("MYSERVICE", "handle");
// Deprecated: Using MYSERVICE as a class name with incorrect case is deprecated,
//             use the correct casing MyService instead
</code>

==== 2.28 ''ReflectionProperty'' constructor ====

Instantiating ''ReflectionProperty'' with a wrong-cased class name emits a deprecation.

<code php>
class MyModel {
    public int $id = 0;
}

// Incorrect — E_DEPRECATED
$rp = new ReflectionProperty("MYMODEL", "id");
// Deprecated: Using MYMODEL as a class name with incorrect case is deprecated,
//             use the correct casing MyModel instead
</code>

==== 2.29 ''ReflectionClassConstant'' constructor ====

Instantiating ''ReflectionClassConstant'' with a wrong-cased class name emits a deprecation.

<code php>
class Status {
    const ACTIVE = 1;
}

// Incorrect — E_DEPRECATED
$rcc = new ReflectionClassConstant("status", "ACTIVE");
// Deprecated: Using status as a class name with incorrect case is deprecated,
//             use the correct casing Status instead
</code>

==== 2.30 ''ReflectionClass::isSubclassOf()'' ====

Passing a wrong-cased class name string to ''isSubclassOf()'' emits a deprecation.

<code php>
class Base {}
class Child extends Base {}

$rc = new ReflectionClass(Child::class);

// Incorrect — E_DEPRECATED
var_dump($rc->isSubclassOf("base"));
// Deprecated: Using base as a class name with incorrect case is deprecated,
//             use the correct casing Base instead
</code>

==== 2.31 ''ReflectionClass::implementsInterface()'' ====

Passing a wrong-cased interface name string to ''implementsInterface()'' emits a deprecation.

<code php>
interface JsonExportable {}
class MyCollection implements JsonExportable {}

$rc = new ReflectionClass(MyCollection::class);

// Incorrect — E_DEPRECATED
var_dump($rc->implementsInterface("jsonexportable"));
// Deprecated: Using jsonexportable as a class name with incorrect case is deprecated,
//             use the correct casing JsonExportable instead
</code>

==== 2.32 ''ReflectionClass::getAttributes()'' with ''IS_INSTANCEOF'' ====

When filtering attributes by a base class using ''IS_INSTANCEOF'', the class name must match its declaration casing.

<code php>
#[Attribute(Attribute::TARGET_CLASS)]
class MyAttr {}

#[MyAttr]
class Foo {}

$rc = new ReflectionClass(Foo::class);

// Incorrect — E_DEPRECATED
$rc->getAttributes("MYATTR", ReflectionAttribute::IS_INSTANCEOF);
// Deprecated: Using MYATTR as a class name with incorrect case is deprecated,
//             use the correct casing MyAttr instead
</code>

==== 2.33 ''ReflectionParameter'' constructor — array callable ====

When constructing a ''ReflectionParameter'' using an array callable, the class name must match its declaration casing.

<code php>
class MyClass {
    public function myMethod(int $x): void {}
}

// Incorrect — E_DEPRECATED
new ReflectionParameter(["MYCLASS", "myMethod"], 0);
// Deprecated: Using MYCLASS as a class name with incorrect case is deprecated,
//             use the correct casing MyClass instead
</code>

==== 2.34 ''ReflectionProperty::isReadable()'' and ''isWritable()'' ====

The optional scope class name string passed to these methods must match the declaration casing.

<code php>
class MyClass {
    protected int $value = 0;
}

$rp = new ReflectionProperty(MyClass::class, "value");

// Incorrect — E_DEPRECATED
$rp->isReadable("MYCLASS");
// Deprecated: Using MYCLASS as a class name with incorrect case is deprecated,
//             use the correct casing MyClass instead
</code>

==== 2.35 ''ReflectionClass::getProperty()'' with fully-qualified name ====

When a property is accessed via its fully-qualified name (e.g., ''"ParentClass::$prop"''), the class name prefix must match the declaration casing.

<code php>
class Base {
    protected int $value = 0;
}
class Child extends Base {}

$rc = new ReflectionClass(Child::class);

// Correct — no warning
$rc->getProperty("Base::value");

// Wrong case — E_DEPRECATED
$rc->getProperty("BASE::value");
// Deprecated: Using BASE as a class name with incorrect case is deprecated,
//             use the correct casing Base instead
</code>

==== 2.36 Magic method declarations ====

Declaring a magic method with wrong case emits a deprecation. Three magic methods have canonical names that differ from their all-lowercase form: ''%%__toString%%'', ''%%__callStatic%%'', and ''%%__debugInfo%%''. All other magic methods (''%%__construct%%'', ''%%__destruct%%'', ''%%__clone%%'', ''%%__get%%'', ''%%__set%%'', ''%%__isset%%'', ''%%__unset%%'', ''%%__call%%'', ''%%__invoke%%'', ''%%__sleep%%'', ''%%__wakeup%%'', ''%%__serialize%%'', ''%%__unserialize%%'', ''%%__set_state%%'') are corrected to their all-lowercase canonical form.

<code php>
// Incorrect — E_DEPRECATED for each
class Foo {
    public function __CONSTRUCT() {}
    // Deprecated: Declaring Foo::__CONSTRUCT() with incorrect case is deprecated,
    //             use the correct casing __construct() instead

    public function __tostring() { return "foo"; }
    // Deprecated: Declaring Foo::__tostring() with incorrect case is deprecated,
    //             use the correct casing __toString() instead

    public static function __CALLSTATIC($name, $args) {}
    // Deprecated: Declaring Foo::__CALLSTATIC() with incorrect case is deprecated,
    //             use the correct casing __callStatic() instead

    public function __SLEEP() { return []; }
    // Deprecated: Declaring Foo::__SLEEP() with incorrect case is deprecated,
    //             use the correct casing __sleep() instead
}
</code>

==== 2.37 File-level ''use'' imports ====

File-level ''use'', ''use function'', and ''use const'' declarations that reference a class, function, or namespace with wrong casing emit a deprecation when the import is first resolved. That happens when the aliased name is first used, not at the ''use'' line itself.

<code php>
namespace MyApp\Service;

class UserService {}
function myHelper(): void {}

// Correct — no warning
use MyApp\Service\UserService;
use function MyApp\Service\myHelper;

// Incorrect — E_DEPRECATED fires the first time UserService or myHelper() is used
use myapp\service\UserService;
// Deprecated: Using myapp\service\UserService as a class name with incorrect case
//             is deprecated, use the correct casing MyApp\Service\UserService instead

use function MYAPP\SERVICE\MYHELPER;
// Deprecated: Calling MYHELPER() is deprecated,
//             use the correct casing MyApp\Service\myHelper() instead
</code>

The ''as'' alias itself is not checked: ''use MyApp\Service\UserService as US'' is fine regardless of what ''US'' is. Only the namespace path being imported is validated.

The check fires at the same resolution point as sections 2.4 and 2.5 (class instantiation and namespace segment validation). The ''use'' import case is called out separately because the wrong casing comes from the import path, not the usage site, so the fix is to correct the ''use'' declaration rather than the call site.

==== 2.38 ''namespace'' declarations ====

When a class is defined, the engine checks whether other classes already registered in the same namespace use a different casing for the namespace prefix. If so, a deprecation is emitted at class-definition time.

<code php>
// file-a.php
namespace MyApp\Service;
class UserService {}  // registers MyApp\Service\UserService

// file-b.php — loaded after file-a.php
namespace MYAPP\Service;  // same namespace as above, but wrong case
class OtherService {}
// Deprecated: Namespace MYAPP\Service uses incorrect casing,
//             the canonical casing is MyApp\Service

// file-c.php
namespace myapp\service;  // also wrong
class ThirdService {}
// Deprecated: Namespace myapp\service uses incorrect casing,
//             the canonical casing is MyApp\Service
</code>

The canonical casing for a namespace is set by the first class registered in it. Subsequent files that declare the same namespace with different casing are inconsistent by definition: their classes will have mismatched ''ce->name'' prefixes and will cause deprecations whenever referenced from code using the canonical casing.

This check fires in ''zend_inheritance.c'' inside ''zend_do_link_class()'', when the class is first resolved at runtime. It scans ''EG(class_table)'' for any previously registered class whose namespace prefix is case-insensitively equal but case-differently from the current class's prefix. The first such match establishes the canonical casing. The scan is O(n) in the number of registered classes, but it executes at most once per class per request (under opcache, only during the initial cache-miss linking pass).

==== 2.39 ''unserialize()'' — objects and enums ====

Serialized object and enum data embeds the class name as a string. If that name uses incorrect casing, a deprecation is emitted on deserialization.

<code php>
class MyRow {
    public string $name = '';
}

// Old serialized data with wrong-case class name — E_DEPRECATED
$obj = unserialize('O:5:"MYROW":1:{s:4:"name";s:5:"hello";}');
// Deprecated: Using MYROW as a class name with incorrect case is deprecated,
//             use the correct casing MyRow instead

enum Suit: string { case Hearts = 'H'; }

$s = unserialize('E:11:"SUIT:Hearts";');
// Deprecated: Using SUIT as a class name with incorrect case is deprecated,
//             use the correct casing Suit instead
</code>

The check fires at all class-resolution paths inside the unserializer: the CE cache fast path, the direct class table hash lookup, the ''zend_lookup_class_ex'' slow path, and the ''unserialize_callback_func'' fallback. Stored data with wrong-case names must be re-serialized before the next major version.

==== 2.40 ''ArrayObject::%%__unserialize%%()'' — iterator class ====

When deserializing an ''ArrayObject'' with a custom iterator class, the stored class name must match the declaration casing.

<code php>
class MyIterator extends ArrayIterator {}

// Old serialized ArrayObject with wrong-case iterator class — E_DEPRECATED
$ao = unserialize('O:11:"ArrayObject":4:{...i:3;s:10:"MYITERATOR";}');
// Deprecated: Using MYITERATOR as a class name with incorrect case is deprecated,
//             use the correct casing MyIterator instead
</code>

==== 2.41 SOAP classmap ====

''SoapServer'' and ''SoapClient'' accept a ''classmap'' option mapping SOAP type names to PHP class names. When the PHP class name value uses incorrect casing, a deprecation is emitted the first time that type is decoded.

<code php>
class BookInfo {
    public string $title = '';
}

$server = new SoapServer('service.wsdl', [
    'classmap' => ['BookInfo' => 'BOOKINFO'],  // wrong case — E_DEPRECATED on decode
]);
// Deprecated: Using BOOKINFO as a class name with incorrect case is deprecated,
//             use the correct casing BookInfo instead
</code>

The check fires in ''ext/soap/php_encoding.c'' inside ''to_zval_object()'', after the classmap entry is resolved to a ''zend_class_entry'' via ''zend_fetch_class''.

==== 2.42 ''ArrayObject::setIteratorClass()'' ====

Setting the iterator class on an ''ArrayObject'' to a wrong-cased name emits a deprecation.

<code php>
class MyArrayIterator extends ArrayIterator {}

$ao = new ArrayObject([1, 2, 3]);
$ao->setIteratorClass("MYARRAYITERATOR");
// Deprecated: Using MYARRAYITERATOR as a class name with incorrect case is deprecated,
//             use the correct casing MyArrayIterator instead

echo $ao->getIteratorClass() . "\n"; // MyArrayIterator
</code>

==== 2.43 ''IteratorIterator'' and ''RecursiveIteratorIterator'' ====

Both ''IteratorIterator'' and ''RecursiveIteratorIterator'' accept an optional second argument to cast the inner iterator to a specific class. If that class name uses incorrect casing, a deprecation is emitted.

<code php>
class MyAggregate implements IteratorAggregate {
    public function getIterator(): ArrayIterator {
        return new ArrayIterator([1, 2, 3]);
    }
}

$it = new IteratorIterator(new MyAggregate(), "MYAGGREGATE");
// Deprecated: Using MYAGGREGATE as a class name with incorrect case is deprecated,
//             use the correct casing MyAggregate instead
</code>

==== 2.44 ''stream_filter_register()'' ====

''stream_filter_register()'' maps a filter name to a PHP class. If the class name uses incorrect casing, a deprecation is emitted when the filter is first applied to a stream.

<code php>
class MyFilter extends php_user_filter {
    public function filter($in, $out, &$consumed, bool $closing): int {
        // ...
        return PSFS_PASS_ON;
    }
}

stream_filter_register("my.upper", "MYFILTER");
// Deprecated fires when the filter is appended to a stream:
// Deprecated: Using MYFILTER as a class name with incorrect case is deprecated,
//             use the correct casing MyFilter instead
</code>

==== 2.45 ''PDO::ATTR_STATEMENT_CLASS'' ====

Setting ''PDO::ATTR_STATEMENT_CLASS'' to a wrong-cased class name emits a deprecation when the first statement is executed with that connection.

<code php>
class MyStatement extends PDOStatement {}

$pdo = new PDO("sqlite::memory:");
$pdo->setAttribute(PDO::ATTR_STATEMENT_CLASS, ["MYSTATEMENT"]);
$stmt = $pdo->query("SELECT 1");
// Deprecated: Using MYSTATEMENT as a class name with incorrect case is deprecated,
//             use the correct casing MyStatement instead
</code>

==== 2.46 ''PDOStatement::setFetchMode(PDO::FETCH_CLASS)'' ====

Passing a wrong-cased class name to ''setFetchMode()'' with ''PDO::FETCH_CLASS'' emits a deprecation when the first row is fetched.

<code php>
class MyRow {
    public mixed $a = null;
}

$stmt = $pdo->prepare("SELECT a FROM t");
$stmt->execute();
$stmt->setFetchMode(PDO::FETCH_CLASS, "MYROW");
$row = $stmt->fetch();
// Deprecated: Using MYROW as a class name with incorrect case is deprecated,
//             use the correct casing MyRow instead
</code>

==== Out of scope ====

The following are **not** affected:

  * Constants (already case-sensitive in PHP)
  * Object properties (already case-sensitive in PHP)
  * Correctly-cased code (zero impact)
  * Autoloaders (unchanged; see "PSR-4 autoloading and filesystem portability" for context)
  * Language keywords (''if'', ''while'', ''self'', ''parent'', ''static'', etc.)
  * Built-in type names in type declarations (''int'', ''string'', ''bool'', ''void'', etc.)

===== Backward Incompatible Changes =====

==== Deprecation warnings emitted (not errors) ====

In PHP 8.6, this is a deprecation warning only. Code continues to work exactly as before. Developers using incorrect casing will see warnings.

^ Version ^ Behavior ^
| PHP 8.5 | ''STRLEN()'' works silently |
| PHP 8.6 | ''STRLEN()'' works, but emits ''E_DEPRECATED'' |
| Next major version | ''STRLEN()'' throws ''E_ERROR'' — fatal, as covered by this RFC |

==== Who is affected? ====

Only code that calls functions/methods or references classes with non-canonical casing.

Affected: ''STRLEN()'', ''new FOO()'', ''$obj->MyMethod()'' when the method is ''myMethod()'', ''$ex instanceof myexception'', wrong-cased type hints, ''new \myapp\service\UserService()'' with wrong-cased namespace, ''use myapp\Service\UserService;'' imports, and ''namespace MYAPP\Service;'' declarations that conflict with ''MyApp\Service'' declared elsewhere.

Not affected: any code following [[https://www.php-fig.org/psr/psr-12/|PSR-12]], [[https://developer.wordpress.org/coding-standards/wordpress-coding-standards/php/|WordPress]], [[https://laravel.com/docs/master/contributions#coding-style|Laravel]], or [[https://symfony.com/doc/current/contributing/code/standards.html|Symfony]] conventions, or anything written with IDE autocomplete.

==== Impact analysis ====

To get a first read on real-world impact, I ran a static scanner against the ten most-downloaded PHP packages and frameworks. The scanner parses each ''.php'' file with ''token_get_all()'' and flags function calls and class references whose name resolves to a PHP built-in (an internal function such as ''strlen'', or an internal class such as ''stdClass'') case-insensitively but does not match it case-sensitively. It covers direct function calls and ''new'' on built-in classes. It does not inspect method calls (''->'' and ''::''), attribute contexts (''#[''), namespace-qualified paths, or function declarations.

The projects scanned, at their current HEAD (June 2026), were: Laravel Framework, Symfony, WordPress, Drupal, PHPUnit, Composer, Guzzle, Doctrine ORM, Carbon, and Monolog.

^ Project ^ Version (HEAD) ^ Wrong-case hits ^ Files affected ^ Details ^
| [[https://github.com/symfony/symfony|Symfony]] | 8.x | **0** | 0 | Clean |
| [[https://github.com/drupal/drupal|Drupal]] | 12.x | **0** | 0 | Clean |
| [[https://github.com/sebastianbergmann/phpunit|PHPUnit]] | 13.x | **0** | 0 | Clean |
| [[https://github.com/composer/composer|Composer]] | 2.x | **0** | 0 | Clean |
| [[https://github.com/guzzle/guzzle|Guzzle]] | 7.x | **0** | 0 | Clean |
| [[https://github.com/doctrine/orm|Doctrine ORM]] | 3.x | **0** | 0 | Clean |
| [[https://github.com/Seldaek/monolog|Monolog]] | 3.x | **0** | 0 | Clean |
| [[https://github.com/laravel/framework|Laravel Framework]] | 13.x | **1** | 1 | ''new StdClass'' in ''SupportHelpersTest.php'' (test file) |
| [[https://github.com/WordPress/WordPress|WordPress]] | 7.x | **7** | 1 | ''Chr()'', ''Ord()'' in bundled ''class-pclzip.php'' (third-party library, ~2003) |
| [[https://github.com/briannesbitt/Carbon|Carbon]] | 3.x | **29** | 1 | ''ucFirst()'' throughout ''phpdoc.php'' (auto-generated doc script, not shipped) |

<To be measured deeper>

For the built-in references it does cover, the total is 37 hits across 3 files, and seven of the ten projects had none. All of them sit in peripheral files: a test file, a legacy bundled third-party library, and an auto-generated tooling script. None are in production application or framework logic.

PSR-4 autoloading on case-sensitive filesystems already prevents class-name case mismatches in active code. The rare violations that remain are in files that predate modern tooling or are not part of a deployable library.

==== Serialized data: a qualitatively different concern ====

The deprecations in sections 2.39 and 2.40 (''unserialize()'' and ''ArrayObject::%%__unserialize%%()'') differ from every other deprecation in this RFC in one critical way: the affected string is not in source code the developer controls. It is in stored data.

With source-code deprecations, ''grep'' or a static analyzer finds every call site. The fix is a one-time refactor with no data migration.

Serialized data is different. The deprecation fires on every row in every cache, session store, or database that contains a PHP-serialized object whose class name has the wrong casing. That data may have been written years ago by code the developer no longer runs. The fix is not a one-time source change but a data migration. Applications relying on serialized objects with wrong-cased class names must re-serialize that data before the next major version. ''class_alias()'' is a short-term bridge (mapping the wrong-cased name to the canonical class) while the migration is in progress.

===== Proposed PHP version(s) =====

Both phases are covered by this RFC and voted on together (see Voting Choices).

**PHP 8.6**

  * Emit ''E_DEPRECATED'' for all identifiers listed in section 2
  * No behavioral change; affected code continues to work

**Next major version**

  * Promote deprecations to ''E_ERROR'' — incorrect casing becomes a fatal error
  * Remove the ''tolower'' pass from the function, method, and class lookup hot path

===== RFC Impact =====

==== Tooling ====

Several tools already flag incorrect casing. Once PHP itself emits ''E_DEPRECATED'', teams have a runtime-backed reason to treat these as CI failures rather than style nits.

**PHPStan** reports class, function, method, static method, interface, trait, and enum name case mismatches via error identifiers such as [[https://phpstan.org/error-identifiers/class.nameCase|class.nameCase]] and [[https://phpstan.org/error-identifiers/function.nameCase|function.nameCase]]. The relevant config options (''checkInternalClassCaseSensitivity'', ''checkFunctionNameCase'') are off by default but enabled by the [[https://github.com/phpstan/phpstan-strict-rules|phpstan/phpstan-strict-rules]] extension.

**PHP-CS-Fixer** has two dedicated fixers: [[https://cs.symfony.com/doc/rules/casing/native_function_casing.html|native_function_casing]] and [[https://cs.symfony.com/doc/rules/casing/class_reference_name_casing.html|class_reference_name_casing]]. Both are included in the ''@PhpCsFixer'' and ''@Symfony'' rulesets.

**PhpStorm** has a built-in inspection [[https://www.jetbrains.com/help/inspectopedia/PhpMethodOrClassCallIsNotCaseSensitiveInspection.html|PhpMethodOrClassCallIsNotCaseSensitiveInspection]] that fires on functions, methods, classes, and namespaces called with different casing than their declarations. It is enabled by default.

**Psalm** does not yet implement case-sensitivity detection natively ([[https://github.com/vimeo/psalm/issues/1174|issue #1174]] from 2019 remains open). A sample plugin is available in the Psalm repository for teams that need it sooner.

**PHPCS** enforces lowercase PHP keywords via [[https://github.com/squizlabs/PHP_CodeSniffer/blob/master/src/Standards/Generic/Sniffs/PHP/LowerCaseKeywordSniff.php|Generic.PHP.LowerCaseKeyword]] but has no sniff for user-defined identifier casing.

**mir** detects wrong-cased identifiers via ''[[http://jorgsowa.me/mir/reference/issues/other/wrong-case-function/|WrongCaseFunction]]'' (MIR1009, function calls), ''[[http://jorgsowa.me/mir/reference/issues/other/wrong-case-method/|WrongCaseMethod]]'' (MIR1010, instance and static method calls), and ''[[http://jorgsowa.me/mir/reference/issues/other/wrong-case-class/|WrongCaseClass]]'' (MIR1011, class names in ''new'', type hints, and static-call positions).

==== Opcache and compile-time checks ====

A small number of deprecations fire at **compile time** rather than at runtime:

  * **Magic method declarations** (section 2.36): the check runs in ''zend_begin_method_decl()'' during compilation.
  * **Compile-time-bound function calls** (section 2.1, partial): when a function is known at compile time and resolved to ''ZEND_INIT_FCALL'', the case check runs during the compilation pass.
  * **Generator return types** (section 2.8): the check runs in ''zend_mark_function_as_generator()'' during compilation.
  * **Compile-time-bound class constant access** (section 2.9, partial): when the class is known at compile time, the check runs in ''zend_compile_class_const()''. Otherwise it runs at runtime.
  * **''%%::class%%'' on a compile-time-known class** (the ''%%::class%%'' name resolution section): the check runs in ''zend_try_compile_const_expr_resolve_class_name()''.

Under opcache, compile-time checks fire **once at warmup** and are never re-triggered from cached bytecode. All other checks fire at runtime on the first execution of each affected call site.

Because these deprecations are emitted during compilation, a ''set_error_handler()'' callback installed at runtime will not see them. Compile-time ''E_DEPRECATED'' is handled while the file is being compiled, before user code in that request runs. They still appear in the error log and are visible to handlers installed earlier, for example in an ''auto_prepend_file''.  All runtime checks are catchable by a normal error handler.

==== Non-ASCII identifiers ====

Case-insensitivity has never applied to non-ASCII identifiers. This RFC does not change that: deprecations fire for ASCII case mismatches only, and Unicode case folding is out of scope.

==== Performance ====

The two phases affect performance differently. PHP 8.6 adds a small cost; the next major version removes one.

In PHP 8.6 the deprecation checks are not free. On a resolution cache miss, each affected call site runs a length comparison and, when the lengths match, a ''zend_binary_strcasecmp'' against the canonical name (''zend_check_func_name_case()'' / ''zend_check_class_name_case()''). The result is then cached in the run-time cache or CE cache, so a warm call site pays nothing on later hits; the cost falls on the first resolution. ''zend_check_namespace_case()'' is heavier: it scans the class table once per class at link time, which is O(n²) over //n// loaded classes (see "Cost of the namespace scan"). For an application with many thousands of classes this warmup cost has not been measured and could be significant. It should be benchmarked, and if it matters, optimized before 8.6 by indexing namespaces in a side table rather than scanning.

The next major version removes the ''tolower'' pass from the function, method, class, and namespace resolution path, and drops the deprecation checks and the namespace scan along with it. The per-call saving is small but happens on every such reference in every request. This is an expected gain, not a measured one.

Memory follows the same split. The 8.6 checks are pure comparisons and allocate nothing persistent. The next major version can reduce memory: today the engine keys the function and class tables by a separate lowercased ''zend_string'' while the canonical casing is also held in ''ce->name'' / ''op_array->function_name'', so once lookups are case-sensitive the tables can be keyed by the canonical name directly, dropping the duplicate lowercased key. That saves roughly one interned ''zend_string'' per distinct class and function name, a small but real reduction in interned-string and hash-table memory for class-heavy applications.


<I still want to mesaure it properly>

==== To Existing Extensions ====

**PHP 8.6 — deprecation signals (compile-time)**

Extensions that look up class entries via ''zend_fetch_class()'' or call functions by name via ''zend_call_function()'' do not need changes. The deprecation fires in the engine's resolution layer. Extensions that construct class name strings internally (e.g. ''ext/soap'' classmap, ''ext/pdo'') must pass the canonically-cased name; the specific cases are covered in sections 2.41–2.46.

To give extension authors a compile-time signal, the two case-insensitive hash-table lookup helpers used to resolve symbol names are marked ''ZEND_ATTRIBUTE_DEPRECATED'' in PHP 8.6 headers:

  * ''zend_hash_find_ptr_lc()''
  * ''zend_hash_str_find_ptr_lc()''

Extensions calling them receive a compiler warning when building against PHP 8.6, and a build failure in the next major version when the functions are removed.

A public/internal split is required because these two functions are also used internally, at 23 call sites in php-src. They cannot just be switched to case-sensitive lookups in 8.6: case-insensitive resolution is still fully supported there (the deprecation is only a warning, and ''STRLEN()'' still resolves to ''strlen()''), so the engine itself still needs case-insensitive lookups. Marking the functions deprecated while the engine still called them would emit deprecation warnings on PHP's own build and break ''--enable-werror'' builds.

**Next major version — removal**

  * The deprecated public ''zend_hash_find_ptr_lc()'' and ''zend_hash_str_find_ptr_lc()'' are removed from the API.
  * The internal inline twins ''_zend_hash_find_ptr_lc()'' / ''_zend_hash_str_find_ptr_lc()'' and their remaining engine call sites are converted to case-sensitive lookups and removed.
  * The transitional helpers ''zend_resolve_function_lc_deprecate_case()'' and the ''str''/''len'' case-check sibling are removed; the case-check helpers ''zend_check_class_name_case()'' / ''zend_check_func_name_case()'' are removed along with the deprecation they emit. Extensions move to plain case-sensitive ''zend_hash_str_find_ptr()'' / ''zend_lookup_class()''.
  * The ''tolower'' pass is removed from the function, method, and class lookup hot paths.
  * Extensions that have not migrated away from the deprecated helpers will fail to compile.

==== To SAPIs ====

No SAPI-specific changes. Deprecation warnings surface through the standard error-reporting mechanism (''E_DEPRECATED'') and are visible in every SAPI: CLI, CGI, FPM, and embedded.

===== Open Issues =====

==== Namespace canonical casing is "first registered wins" ====

The canonical casing for a namespace (section 2.38) is not declared anywhere — it is established by the first class registered in that namespace, as encountered while scanning ''EG(class_table)''. If the first class loaded in a namespace happens to use non-canonical casing, that casing becomes the reference, and correctly-cased classes loaded later are the ones flagged. Because class load order depends on autoloading, execution path, and opcache preloading, the "canonical" casing — and therefore which file is reported — can vary between runs. There is no authoritative source of truth for namespace casing.

==== Cost of the namespace scan ====

''zend_check_namespace_case()'' scans the entire class table to find a previously registered class in the same namespace. It runs once per class at link time, so loading ''n'' classes is O(n²) in the aggregate. Under opcache the scan still runs during the initial cache-miss linking pass. The per-class cost is bounded but the warmup cost for applications with many thousands of classes has not yet been measured (see Performance).

==== Possible double emission of the namespace deprecation ====

''zend_check_namespace_case()'' is invoked from four sites — ''zend_do_link_class()'' and ''zend_try_early_bind()'' (''Zend/zend_inheritance.c''), and ''zend_bind_class_in_slot()'' and ''zend_compile_class_decl()'' (''Zend/zend_compile.c'', early-bound classes). Depending on the binding path a class takes, the namespace deprecation could be emitted more than once for the same class. This needs a test to confirm it fires exactly once per class per request.

==== Error handler reentrancy in ''catch'' clauses ====

The case check for ''catch'' clauses (section 2.11) runs while an exception is pending. The implementation temporarily clears ''EG(exception)'' so ''zend_error()'' can invoke a userland error handler, then restores it. If that handler throws, the new exception is discarded when the original is restored. This edge case should be reviewed.

==== ''%%::class%%'' is only checked when the class is known at compile time ====

''MyClass::class'' is checked against the declaration when the class is already declared while the referencing file compiles (see "''%%::class%%'' name resolution"). When the class is autoloaded later, the ''%%::class%%'' site folds to the as-written string with no warning, and the mismatch surfaces only when that string reaches a consumer that does a real lookup (''new'', ''class_exists()'', Reflection, and so on). A string used purely as a string, such as a log line or an array key, keeps its wrong casing with no signal.

Making ''%%::class%%'' itself warn in every case would mean replacing the compile-time fold with a runtime registry lookup. That costs performance on a very common construct and only warns intermittently depending on load order, which is not a worthwhile trade for a deprecation. Static analyzers ([[https://phpstan.org/error-identifiers/class.nameCase|PHPStan]], [[https://www.jetbrains.com/help/inspectopedia/PhpMethodOrClassCallIsNotCaseSensitiveInspection.html|PhpStorm]]) already flag ''%%::class%%'' case mismatches today.

(First-class callable syntax — ''strlen(...)'', ''Foo::bar(...)'', ''$obj->method(...)'' — //is// covered: the case check runs on the resolved function/method, so wrong casing in a first-class callable is flagged like any other call.)

----

===== Future Scope =====

==== Out of scope for this RFC ====

  * Changing built-in constant casing
  * Changing reserved keyword casing
  * Array inputs to built-in functions (e.g. the ''$options'' array accepted by ''setcookie()'')

===== Voting Choices =====

Vote 1 requires a 2/3 majority; Vote 2 requires a 1/2 majority per the [[https://wiki.php.net/rfc/voting|PHP voting policy]]. Voting opens and closes at the same time for both questions.

**Vote 1: PHP 8.6 — Deprecation**

<doodle title="Emit E_DEPRECATED for case-insensitive function, method, class, and namespace references in PHP 8.6?" voteType="single" closed="true">
   * Yes
   * No
   * Abstain
</doodle>

**Vote 2: PHP 9.0 — Enforcement**

<doodle title="Promote the deprecation to E_ERROR (fatal) in PHP 9.0?" voteType="single" closed="true">
   * Yes
   * No
   * Abstain
</doodle>

===== Implementation =====

Implementation is in progress.

  * [[https://github.com/php/php-src/pull/22260|Fatal error PR]]
  * Deprecation PR

===== Prior art and previous discussions =====

Case sensitivity has come up on php-internals repeatedly since 2003. None of those discussions produced a merged RFC for functions or classes. What they did produce is a clear record of which arguments have been tried, which got traction, and which constraints keep resurfacing.

==== Discussions ====

=== PHP Bug #26575 — "Case Sensitive Class Names" (2003) ===

The earliest recorded request. Closed as Won't Fix by Andrey Zmievski: "This will break enormous number of applications." ([[https://bugs.php.net/bug.php?id=26575|bugs.php.net/bug.php?id=26575]])

=== "Complete case-sensitivity in PHP" — php-internals, April 2012 ===

([[https://externals.io/message/60228|externals.io/message/60228]]) Key participants: Nikita Popov, Matthew Weier O'Phinney, Yasuo Ohgaki, Galen Wright-Watson.

Established that ''tolower_map'' in ''zend_operators.c'' is ASCII-only and must be removed rather than extended. Matthew Weier O'Phinney cautioned against runtime ini options that change identifier resolution behavior.

=== PHP Bug #62655 — "Request: optional class name case-sensitivity" (2012) ===

A follow-up bug proposing a three-level ''php.ini'' option. Suspended in April 2020 by cmb@php.net as "controversial, requiring internals list discussion." ([[https://bugs.php.net/bug.php?id=62655|bugs.php.net/bug.php?id=62655]])

=== "PHP and case-sensitivity inconsistency" — php-internals, January 2014 ===

([[https://externals.io/message/71592|externals.io/message/71592]]) Stas Malyshev made the most durable argument from this period: any change must be all-or-nothing. A partial change creates a worse inconsistency than the current uniform behavior. A ''use strict'' pragma modeled on JavaScript ES5 was proposed as a migration-friendly path; it never advanced.

=== RFC: "Make the PHP core case-sensitive" — François Laupretre (2014) ===

([[https://wiki.php.net/rfc/case-sensitivity|wiki.php.net/rfc/case-sensitivity]]) The only RFC draft ever written on this topic, other than the present one. Proposed making namespaces, classes, interfaces, traits, functions, and non-magic methods all case-sensitive. Left incomplete, never voted on, status: Inactive.

=== "Proposal for PHP 7: case-sensitive symbols" — php-internals, December 2014 ===

([[https://externals.io/message/79824|externals.io/message/79824]]) Key participants: François Laupretre, Andrea Faulds, Marco Pivetta, Pierre Joye, Ferenc Kovacs.

Proposed pairing the PHP 7 release with case-sensitivity enforcement (E_STRICT in 7.x, removal in 8.0). BC cost and ecosystem disruption dominated the opposition. No change was made in PHP 7.

=== RFC: "Deprecate and Remove Case-Insensitive Constants" — Nikita Popov (2018) ===

([[https://wiki.php.net/rfc/case_insensitive_constant_deprecation|wiki.php.net/rfc/case_insensitive_constant_deprecation]]) The only successful related RFC. Targeted only the ''define()'' ''$case_insensitive'' flag; the RFC text explicitly excluded functions and classes. Passed; implemented as PHP 7.3 deprecation and PHP 8.0 removal. Sara Golemon supported deprecating constants but said extending the change to functions and classes "would be a much more aggressive movement."

=== "Revisiting case-sensitivity in PHP" — php-internals, June 2024 ===

([[https://externals.io/message/123573|externals.io/message/123573]]) Key participants: Valentin Udaltsov (initiator), Ben Ramsey, Levi Morrison, Gina P. Banyard, Timo Tijhof (Wikimedia).

  * Levi Morrison stated PHP 9.0 is the right enforcement target: "This isn't some minor thing squirreled away in a library — this is the core language."
  * Gina P. Banyard raised namespace canonicalization (the class table stores lowercased keys; retrieving the canonical-cased name requires reading ''ce->name'').
  * Timo Tijhof noted that Wikimedia stores PHP 4-era serialized data with lowercase class names; ''class_alias()'' was suggested as a migration path.
  * Udaltsov cited Nikita Popov's suggestion to start with "class and class-like names first, then extend to functions", which is the ordering this RFC follows.

No RFC was drafted from this thread. The informal consensus was: PHP 9.0 enforcement, bundling namespaces + classes + functions in a single change.

==== Pull requests ====

No PR targeting function, method, or class name case has ever been merged; merged entries all relate to constants.

^ PR ^ Author ^ Status ^ Notes ^
| [[https://github.com/php/php-src/pull/965|#965]] (2014) | flaupretre | Closed, never merged | ''E_STRICT'' on class/function case mismatch — the only prior PR directly targeting function and class name case sensitivity |
| [[https://github.com/php/php-src/pull/3321|#3321]] (2018) | nikic | **Merged** | Implements the accepted RFC: deprecates ''define()'' ''$case_insensitive'' flag. Scoped strictly to constants |
| [[https://github.com/php/php-src/pull/3770|#3770]] (2019) | nikic | Closed WIP | Broad WIP collecting PHP 8 removals, including case-insensitive constants |
| [[https://github.com/php/php-src/pull/3833|#3833]] (2019) | cmb69 | **Merged** | Extends PHP 7.3 constant deprecation to ''com_load_typelib()'' and ''com.autoregister_casesensitive'' INI |
| [[https://github.com/php/php-src/pull/3836|#3836]] (2019) | cmb69 | Closed | Companion to #3833: full removal of ''$case_sensitive'' parameter and INI setting |
| [[https://github.com/php/php-src/pull/9439|#9439]] (2022), [[https://github.com/php/php-src/pull/9685|#9685]] (2023), [[https://github.com/php/php-src/pull/17071|#17071]] (2024) | kocsismate / jorgsowa / DanielEScherzer | Merged / Open | ''CONST_CS'' cleanup series: stop generating it in stubs, remove usages, remove from API surface |

===== Rejected Alternatives =====

==== Make it an error immediately ====

Skipping a deprecation period and going straight to an error would break existing code without warning, which violates PHP's deprecation policy.

==== php.ini option ====

A ''php.ini'' option to enable or disable case-sensitivity enforcement was proposed in [[https://bugs.php.net/bug.php?id=62655|Bug #62655 (2012)]] and revisited in the [[https://externals.io/message/71592|2014 internals thread]]. It was rejected on both occasions. Matthew Weier O'Phinney cautioned explicitly against runtime ini options that change identifier resolution behavior, and Stas Malyshev argued that any partial change creates a worse inconsistency than the current uniform behavior. An ini option would produce code that behaves differently depending on server configuration, which is the opposite of what this RFC is trying to fix.

===== References =====
  * [[https://www.php-fig.org/psr/psr-12/|PSR-12: Extended Coding Style Guide]]

===== Changelog =====

  * 1.0 — Initial draft.
