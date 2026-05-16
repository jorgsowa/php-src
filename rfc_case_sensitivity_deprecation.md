====== PHP RFC: Case-sensitive PHP ======

  * Version: 1.0
  * Date: 2026-06-01
  * Author: Jorg Sowa <jorg.sowa@gmail.com>
  * Status: Draft
  * Implementation: [[https://github.com/jorgsowa/php-src/tree/feat/case-sensitive-class-names]]
  * Discussion thread: TBD
  * Voting thread: TBD

----

===== 1. Introduction =====

PHP has always treated function, method, and class names as case-insensitive. That was a reasonable early design decision, but it now creates inconsistency without much benefit.

Today, all of these are valid PHP:

<code php>
strlen("hello")     // canonical
STRLEN("hello")     // works
StrLen("hello")     // also works
</code>

Same with classes:

<code php>
new MyClass()       // canonical
new MYCLASS()       // works
new myclass()       // also works
</code>

This RFC proposes emitting ''E_DEPRECATED'' warnings in PHP 8.6 when functions, methods, or classes are referenced with incorrect casing. The goal is to give developers and tools time to adapt before a potential hard break in the next major version.

==== 1.1 Case sensitivity in PHP today ====

PHP is already partially case-sensitive. This RFC addresses the remaining inconsistencies. Here is the full picture:

**Case-insensitive (as of PHP 8.5):**

^ Identifier ^ Note ^
| Function names (user-defined and built-in) | deprecated by this RFC |
| Method names | deprecated by this RFC |
| Class, interface, and trait names | deprecated by this RFC |
| Magic method names (''%%__construct%%'', ''%%__toString%%'') | deprecated by this RFC |
| Namespace names in class references and ''use'' imports | deprecated by this RFC |
| ''namespace'' declarations (inconsistent casing across files) | deprecated by this RFC |
| Keywords (''if'', ''else'', ''for'', ''while'', ''class'', ...) | |
| ''true'', ''false'', ''null'' | |

**Case-sensitive (already enforced):**

^ Identifier ^ Example ^
| Variables | ''$foo'' != ''$Foo'' |
| Constants | ''FOO'' != ''foo'', fatal error on mismatch |
| Object properties | ''$obj->name'' != ''$obj->Name'' |
| Array keys | ''"key"'' != ''"Key"'' |
| Enum cases | ''Color::Red'' != ''Color::red'', fatal error |
| Goto labels | ''myLabel'' != ''MYLABEL'' |

After this RFC is fully enforced in the next major version, all user-defined identifiers in PHP will be case-sensitive. The remaining case-insensitive constructs will all be language-defined: control-flow keywords (''if'', ''while'', ''match'', etc.), built-in type names in type declarations (''int'', ''string'', ''bool'', ''void'', etc.), the special class references ''self'', ''parent'', and ''static'', and the literals ''true'', ''false'', ''null''.

==== 1.2 Language comparison ====

PHP is one of the few remaining languages that does not enforce case sensitivity for user-defined identifiers.

^ Language ^ Case-sensitive? ^ Notes ^
| [[https://docs.python.org/3/reference/lexical_analysis.html#identifiers|Python]] | Yes | |
| [[https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Lexical_grammar|JavaScript]] | Yes | |
| [[https://www.typescriptlang.org/docs/handbook/2/basic-types.html|TypeScript]] | Yes | |
| [[https://ruby-doc.org/docs/ruby-doc-bundle/Manual/man-1.4/syntax.html|Ruby]] | Yes | Capitalized identifiers are constants |
| [[https://go.dev/ref/spec#Exported_identifiers|Go]] | Yes | Case determines visibility: uppercase = exported (public), lowercase = unexported (private) |
| [[https://doc.rust-lang.org/reference/identifiers.html|Rust]] | Yes | Compiler warns on convention violations (''snake_case'' vs ''CamelCase'') |
| [[https://docs.oracle.com/javase/specs/jls/se21/html/jls-3.html#jls-3.8|Java]] | Yes | |
| [[https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure|C#]] | Yes | Guidelines discourage names differing only in case (CLR interop) |
| [[https://docs.swift.org/swift-book/documentation/the-swift-programming-language/lexicalstructure|Swift]] | Yes | |
| [[https://kotlinlang.org/spec/syntax-and-grammar.html|Kotlin]] | Yes | |
| [[https://perldoc.perl.org/perlsyn|Perl]] | Yes | |
| [[https://www.lua.org/manual/5.4/manual.html#3.1|Lua]] | Yes | |
| [[https://cran.r-project.org/doc/manuals/r-release/R-lang.html#Identifiers|R]] | Yes | |
| [[https://learn.microsoft.com/en-us/dotnet/visual-basic/programming-guide/language-features/declared-elements/declared-element-names|Visual Basic / VBA]] | No | Classic VB (1991) and VB.NET (2002); IDE normalizes casing for display |
| [[https://www.freepascal.org/docs-html/ref/refse5.html|Pascal / Delphi]] | No | Case-insensitive since the 1970s |
| [[https://www.ibm.com/docs/en/cobol-zos/6.4?topic=structure-user-defined-words|COBOL]] | No | Keywords and identifiers only; string literals are case-sensitive |
| [[https://helpx.adobe.com/coldfusion/developing-applications/the-cfml-programming-language/elements-of-cfml/about-cfml-variables.html|ColdFusion / CFML]] | No | 1995; tags, functions, and variable names are case-insensitive |
| [[https://learn.microsoft.com/en-us/powershell/scripting/lang-spec/chapter-03?view=powershell-7.4#312-identifiers|PowerShell]] | No | 2006; cmdlets, functions, and variable names are case-insensitive |
| PHP (pre-8.6) | No (partial) | See section 1.1 |

Among general-purpose languages with broad industry adoption, case sensitivity is the norm. Go is the most instructive: it did not just enforce consistent casing, it made the first letter semantically meaningful (access control). PHP cannot reasonably do that at this stage, but removing the implicit lowercasing brings it in line with the mainstream.

The case-insensitive languages in the table split into two groups. Pascal, Delphi, COBOL, and classic Visual Basic predate PHP 3 (1997) and established the convention PHP inherited. ColdFusion (1995) and PowerShell (2006) are domain-specific tools — a web templating language and a shell — where the case-insensitive tradition carried over from earlier scripting environments. VB.NET (2002) postdates PHP 3 but is the direct successor of classic Visual Basic and retains the behavior for backward compatibility. None of these languages are the general-purpose, server-side peers PHP competes with today.

==== 1.3 PSR-4 autoloading and filesystem portability ====

PHP's case-insensitive class lookup interacts badly with [[https://www.php-fig.org/psr/psr-4/|PSR-4 autoloading]] and filesystem case sensitivity in a way that hides bugs during development and surfaces them in production.

PSR-4 maps a fully-qualified class name directly to a file path: ''App\Service\UserService'' → ''app/Service/UserService.php''. The autoloader constructs that path from the class name as written at the call site, then opens the file. Whether that file open succeeds depends entirely on the filesystem.

^ Environment ^ Filesystem ^ ''new app\service\USERSERVICE()'' ^
| Linux (production) | ext4, btrfs (case-sensitive) | Autoloader fails — file not found |
| macOS (developer) | HFS+ / APFS case-insensitive (default) | Autoloader succeeds — file found |
| Windows (developer) | NTFS (case-insensitive) | Autoloader succeeds — file found |

The result is a class of bugs that passes silently on developer machines and breaks only on Linux servers. A wrong-cased ''new APP\SERVICE\USERSERVICE()'' works fine locally, passes CI if CI also runs on macOS or Windows, and then throws a fatal error on the production host.

There is a second subtlety: if the class is already in PHP's class registry (loaded earlier in the same request via a correctly-cased reference), PHP's case-insensitive lookup resolves the wrong-cased reference without ever calling the autoloader. That makes the bug intermittent — it disappears when the class happens to be loaded first by another code path, and reappears when the execution order changes.

This RFC's deprecation warning fires at the PHP engine level regardless of whether the autoloader was involved, which catches both cases.

----

===== 2. Proposal =====

Emit ''E_DEPRECATED'' when any of the following identifiers are referenced with incorrect casing:

**Calls** (2.1–2.3)
  * Function calls — user-defined and built-in (section 2.1)
  * Method calls — instance methods, including dynamic calls (section 2.2)
  * Static method calls (section 2.3)

**Language constructs** (2.4–2.11)
  * Class instantiation via ''new'' (section 2.4)
  * Namespace segments in class references (section 2.5)
  * ''instanceof'' checks (section 2.6)
  * Type declarations — parameter types, return types, and property types (section 2.7)
  * Catch clauses (section 2.8)
  * ''extends'' — wrong-cased parent class name (section 2.9)
  * ''implements'' — wrong-cased interface name (section 2.10)
  * ''use'' — wrong-cased trait name in a class body (section 2.11)

**Callables and dynamic dispatch** (2.12–2.13)
  * Callable class names — array callables ''["ClassName", "method"]'' and string callables ''"ClassName::method"'' (section 2.12)
  * ''Closure::bind()'' and ''bindTo()'' — wrong-cased scope class name (section 2.13)

**Class and function introspection** (2.14–2.19)
  * ''class_exists()'', ''interface_exists()'', ''trait_exists()'', ''enum_exists()'' (section 2.14)
  * ''class_alias()'' — wrong-cased original class name (section 2.15)
  * ''is_a()'' and ''is_subclass_of()'' — wrong-cased class name argument (section 2.16)
  * ''class_parents()'', ''class_implements()'', ''class_uses()'' — wrong-cased class name (section 2.17)
  * ''property_exists()'' — wrong-cased class name string (section 2.18)
  * ''method_exists()'' — wrong-cased class name string (section 2.19)

**Reflection API** (2.20–2.31)
  * ''ReflectionClass'' constructor — wrong-cased class name argument (section 2.20)
  * ''ReflectionAttribute::newInstance()'' — wrong-cased attribute class name (section 2.21)
  * ''ReflectionFunction'' constructor — wrong-cased function name (section 2.22)
  * ''ReflectionMethod'' constructor — wrong-cased class name (section 2.23)
  * ''ReflectionProperty'' constructor — wrong-cased class name (section 2.24)
  * ''ReflectionClassConstant'' constructor — wrong-cased class name (section 2.25)
  * ''ReflectionClass::isSubclassOf()'' — wrong-cased class name (section 2.26)
  * ''ReflectionClass::implementsInterface()'' — wrong-cased interface name (section 2.27)
  * ''ReflectionClass::getAttributes()'' with ''IS_INSTANCEOF'' — wrong-cased class name (section 2.28)
  * ''ReflectionParameter'' constructor — wrong-cased class name in array callable (section 2.29)
  * ''ReflectionProperty::isReadable()'' and ''isWritable()'' — wrong-cased scope class name (section 2.30)
  * ''ReflectionClass::getProperty()'' with ''"ClassName::$prop"'' syntax — wrong-cased class name prefix (section 2.31)

**Declarations** (2.32–2.34)
  * Magic method declarations — declaring ''%%__CONSTRUCT%%'', ''%%__toString%%'', ''%%__sleep%%'', etc. with wrong case (section 2.32)
  * File-level ''use'' imports — wrong-cased class or namespace path in ''use'', ''use function'', and ''use const'' declarations (section 2.33)
  * ''namespace'' declarations — inconsistent namespace casing across files in the same namespace (section 2.34)

**Serialization** (2.35–2.36)
  * ''unserialize()'' — wrong-cased class or enum name in serialized object/enum data (section 2.35)
  * ''ArrayObject::%%__unserialize%%()'' — wrong-cased iterator class name in serialized data (section 2.36)

**Extensions and SPL** (2.37–2.42)
  * ''SoapServer''/''SoapClient'' classmap — wrong-cased PHP class name in ''classmap'' option (section 2.37)
  * ''ArrayObject::setIteratorClass()'' — wrong-cased iterator class name (section 2.38)
  * ''IteratorIterator'' and ''RecursiveIteratorIterator'' — wrong-cased inner iterator class cast (section 2.39)
  * ''stream_filter_register()'' — wrong-cased filter class name (section 2.40)
  * ''PDO::ATTR_STATEMENT_CLASS'' — wrong-cased statement class name (section 2.41)
  * ''PDOStatement::setFetchMode(PDO::FETCH_CLASS)'' — wrong-cased fetch class name (section 2.42)

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

Every segment of a fully-qualified class name must match its declaration, including the namespace prefix. This applies to both literal fully-qualified names and names resolved from ''use'' imports (see section 2.33).

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

==== 2.8 Catch clauses ====

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

==== 2.9 ''extends'' ====

The parent class name in an ''extends'' clause must match its declaration casing.

<code php>
class BaseRepository {}

// Incorrect — E_DEPRECATED
class UserRepository extends BASEREPOSITORY {}
// Deprecated: Using BASEREPOSITORY as a class name with incorrect case is deprecated,
//             use the correct casing BaseRepository instead
</code>

==== 2.10 ''implements'' ====

Interface names in ''implements'' clauses must match their declaration casing.

<code php>
interface Serializable {}

// Incorrect — E_DEPRECATED
class User implements SERIALIZABLE {}
// Deprecated: Using SERIALIZABLE as a class name with incorrect case is deprecated,
//             use the correct casing Serializable instead
</code>

==== 2.11 ''use'' (trait) ====

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

==== 2.12 Callable class names ====

Class names used as part of callables — both array form and ''"Class::method"'' string form — are checked.

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

==== 2.13 ''Closure::bind()'' and ''bindTo()'' ====

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

==== 2.14 ''class_exists()'' family ====

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

==== 2.15 ''class_alias()'' ====

The original class name argument to ''class_alias()'' must match the declaration casing.

<code php>
class UserService {}

// Incorrect — E_DEPRECATED
class_alias("userservice", "US");
// Deprecated: Using userservice as a class name with incorrect case is deprecated,
//             use the correct casing UserService instead
</code>

==== 2.16 ''is_a()'' and ''is_subclass_of()'' ====

When a class name string is passed as the second argument to ''is_a()'' or ''is_subclass_of()'', the casing must match the declaration.

<code php>
class BaseModel {}
class User extends BaseModel {}

$user = new User();

// Correct — no warning
var_dump(is_a($user, 'BaseModel'));
var_dump(is_subclass_of($user, 'BaseModel'));

// Incorrect — E_DEPRECATED
var_dump(is_a($user, 'basemodel'));
// Deprecated: Using basemodel as a class name with incorrect case is deprecated,
//             use the correct casing BaseModel instead
</code>

==== 2.17 ''class_parents()'', ''class_implements()'', ''class_uses()'' ====

The class name string argument to these SPL functions must match the declaration casing.

<code php>
class Base {}
class Child extends Base {}

// Incorrect — E_DEPRECATED
class_parents("CHILD");
// Deprecated: Using CHILD as a class name with incorrect case is deprecated,
//             use the correct casing Child instead
</code>

==== 2.18 ''property_exists()'' ====

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

==== 2.19 ''method_exists()'' ====

When the first argument is a class name string, the casing must match the declaration.

<code php>
class MyService {
    public function handle(): void {}
}

// Incorrect — E_DEPRECATED
method_exists("MYSERVICE", "handle");
// Deprecated: Using MYSERVICE as a class name with incorrect case is deprecated,
//             use the correct casing MyService instead
</code>

==== 2.20 ''ReflectionClass'' constructor ====

Instantiating ''ReflectionClass'' with a wrong-cased class name string emits a deprecation.

<code php>
class MyModel {}

// Incorrect — E_DEPRECATED
$rc = new ReflectionClass("mymodel");
// Deprecated: Using mymodel as a class name with incorrect case is deprecated,
//             use the correct casing MyModel instead
</code>

==== 2.21 ''ReflectionAttribute::newInstance()'' ====

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

==== 2.22 ''ReflectionFunction'' constructor ====

Instantiating ''ReflectionFunction'' with a wrong-cased function name emits a deprecation.

<code php>
function myFunc(): int { return 42; }

// Incorrect — E_DEPRECATED
$rf = new ReflectionFunction("MYFUNC");
// Deprecated: Calling MYFUNC() is deprecated, use the correct casing myFunc() instead
</code>

==== 2.23 ''ReflectionMethod'' constructor ====

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

==== 2.24 ''ReflectionProperty'' constructor ====

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

==== 2.25 ''ReflectionClassConstant'' constructor ====

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

==== 2.26 ''ReflectionClass::isSubclassOf()'' ====

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

==== 2.27 ''ReflectionClass::implementsInterface()'' ====

Passing a wrong-cased interface name string to ''implementsInterface()'' emits a deprecation.

<code php>
interface Countable {}
class MyCollection implements Countable {}

$rc = new ReflectionClass(MyCollection::class);

// Incorrect — E_DEPRECATED
var_dump($rc->implementsInterface("countable"));
// Deprecated: Using countable as a class name with incorrect case is deprecated,
//             use the correct casing Countable instead
</code>

==== 2.28 ''ReflectionClass::getAttributes()'' with ''IS_INSTANCEOF'' ====

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

==== 2.29 ''ReflectionParameter'' constructor — array callable ====

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

==== 2.30 ''ReflectionProperty::isReadable()'' and ''isWritable()'' ====

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

==== 2.31 ''ReflectionClass::getProperty()'' with fully-qualified name ====

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

==== 2.32 Magic method declarations ====

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

==== 2.33 File-level ''use'' imports ====

File-level ''use'', ''use function'', and ''use const'' declarations that reference a class, function, or namespace with wrong casing emit a deprecation when the import is first resolved — that is, when the aliased name is first used, not at the ''use'' line itself.

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

The ''as'' alias itself is not checked — ''use MyApp\Service\UserService as US'' is fine regardless of what ''US'' is. Only the namespace path being imported is validated.

The check fires at the same resolution point as sections 2.4 and 2.5 (class instantiation and namespace segment validation). The ''use'' import case is called out explicitly here because the source of the wrong casing is the import path, not the usage site — fixing the ''use'' declaration is the correct action, not the call site.

==== 2.34 ''namespace'' declarations ====

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

==== 2.35 ''unserialize()'' — objects and enums ====

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

The check fires at all class-resolution paths inside the unserializer: the CE cache fast path, the direct class table hash lookup, the ''zend_lookup_class_ex'' slow path, and the ''unserialize_callback_func'' fallback. Stored data with wrong-case names must be re-serialized before PHP 9.0.

==== 2.36 ''ArrayObject::%%__unserialize%%()'' — iterator class ====

When deserializing an ''ArrayObject'' with a custom iterator class, the stored class name must match the declaration casing.

<code php>
class MyIterator extends ArrayIterator {}

// Old serialized ArrayObject with wrong-case iterator class — E_DEPRECATED
$ao = unserialize('O:11:"ArrayObject":4:{...i:3;s:10:"MYITERATOR";}');
// Deprecated: Using MYITERATOR as a class name with incorrect case is deprecated,
//             use the correct casing MyIterator instead
</code>

==== 2.37 SOAP classmap ====

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

==== 2.38 ''ArrayObject::setIteratorClass()'' ====

Setting the iterator class on an ''ArrayObject'' to a wrong-cased name emits a deprecation.

<code php>
class MyArrayIterator extends ArrayIterator {}

$ao = new ArrayObject([1, 2, 3]);
$ao->setIteratorClass("MYARRAYITERATOR");
// Deprecated: Using MYARRAYITERATOR as a class name with incorrect case is deprecated,
//             use the correct casing MyArrayIterator instead

echo $ao->getIteratorClass() . "\n"; // MyArrayIterator
</code>

==== 2.39 ''IteratorIterator'' and ''RecursiveIteratorIterator'' ====

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

==== 2.40 ''stream_filter_register()'' ====

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

==== 2.41 ''PDO::ATTR_STATEMENT_CLASS'' ====

Setting ''PDO::ATTR_STATEMENT_CLASS'' to a wrong-cased class name emits a deprecation when the first statement is executed with that connection.

<code php>
class MyStatement extends PDOStatement {}

$pdo = new PDO("sqlite::memory:");
$pdo->setAttribute(PDO::ATTR_STATEMENT_CLASS, ["MYSTATEMENT"]);
$stmt = $pdo->query("SELECT 1");
// Deprecated: Using MYSTATEMENT as a class name with incorrect case is deprecated,
//             use the correct casing MyStatement instead
</code>

==== 2.42 ''PDOStatement::setFetchMode(PDO::FETCH_CLASS)'' ====

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

==== 2.43 Scope of non-application ====

The following are **not** affected:

  * Constants (already case-sensitive in PHP)
  * Object properties (already case-sensitive in PHP)
  * Correctly-cased code (zero impact)
  * Autoloaders (unchanged; see section 1.3 for the filesystem portability context)
  * Language keywords (''if'', ''while'', ''self'', ''parent'', ''static'', etc.)
  * Built-in type names in type declarations (''int'', ''string'', ''bool'', ''void'', etc.)

----

===== 3. Backward Incompatible Changes =====

==== 3.1 Deprecation warnings emitted (not errors) ====

In PHP 8.6, this is a deprecation warning only. Code continues to work exactly as before. Developers using incorrect casing will see warnings when running with ''error_reporting(E_ALL)''.

^ Version ^ Behavior ^
| PHP 8.5 | ''STRLEN()'' works silently |
| PHP 8.6 | ''STRLEN()'' works, but emits ''E_DEPRECATED'' |
| PHP 9.0 | May become ''E_ERROR'' (not guaranteed; requires separate RFC) |

==== 3.2 Who is affected? ====

Only code that calls functions/methods or references classes with non-canonical casing.

Affected: ''STRLEN()'', ''new FOO()'', ''$obj->MyMethod()'' when the method is ''myMethod()'', ''$ex instanceof myexception'', wrong-cased type hints, ''new \myapp\service\UserService()'' with wrong-cased namespace, ''use myapp\Service\UserService;'' imports, and ''namespace MYAPP\Service;'' declarations that conflict with ''MyApp\Service'' declared elsewhere.

Not affected: any code following [[https://www.php-fig.org/psr/psr-12/|PSR-12]], [[https://developer.wordpress.org/coding-standards/wordpress-coding-standards/php/|WordPress]], [[https://laravel.com/docs/master/contributions#coding-style|Laravel]], or [[https://symfony.com/doc/current/contributing/code/standards.html|Symfony]] conventions — and anything written with IDE autocomplete.

==== 3.3 Impact analysis ====

Mixed-casing issues appear primarily in older codebases. Modern frameworks enforce correct casing already, and anything running a recent PHPStan or PHP-CS-Fixer config will have caught these before this RFC fires.

==== 3.4 Serialized data: a qualitatively different concern ====

The deprecations in sections 2.35 and 2.36 (''unserialize()'' and ''ArrayObject::%%__unserialize%%()'') differ from every other deprecation in this RFC in one critical way: the affected string is not in source code the developer controls — it is in stored data.

With source-code deprecations, ''grep'' or a static analyzer finds every call site. The fix is a one-time refactor with no data migration.

Serialized data is different. The deprecation fires on every row in every cache, session store, or database that contains a PHP-serialized object whose class name has the wrong casing — data written potentially years ago by code the developer no longer runs. The consequences differ:

^ Property ^ Source-code deprecations ^ Serialized data deprecations ^
| Affected strings are in... | ''.php'' files | cache rows, database columns, session files |
| Developer controls the strings? | Yes | Not after data was written |
| Fix is... | Edit source, deploy | Re-serialize every affected row before PHP 9.0 |
| Deprecation fires... | Per call in code | Per read from storage |
| Silenced with ''@''? | Yes | Yes, but silences all errors in that block |
| Tooling can find all instances? | Yes (static analysis) | Only with a full data scan |

In practice, most applications that serialize objects store canonical class names, because serialization uses ''get_class()'' which always returns the correctly-cased name. Wrong-case serialized data can only exist if:

  - The data was written by PHP 4-era code that stored lowercased class names (a PHP 4 behavior PHP 5 changed in 2004).
  - The serialized string was constructed manually or by a non-PHP tool that got the casing wrong.
  - An application intentionally called ''unserialize()'' on untrusted or externally-provided data containing arbitrary class names.

Applications in any of these categories should audit their stored data before upgrading to the PHP 9.0 enforcement. The ''unserialize_callback_func'' ini option or a custom ''unserialize()'' wrapper can be used during migration to detect and re-serialize affected rows.

This behavior may warrant a separate discussion on php-internals, independent of the broader case-sensitivity RFC, before it is finalized.

----

===== 4. Proposed PHP version(s) =====

**Target: PHP 8.6** (estimated November 2026)

  * Emit ''E_DEPRECATED'' warnings
  * Gives developers and tools time to adapt

**Future evolution (not part of this RFC):**

PHP 9.0 (estimated 2029):
  * Consider making case-insensitivity an error
  * Requires a separate RFC and vote
  * Not guaranteed; depends on ecosystem feedback

----

===== 5. RFC Impact =====

==== 5.1 Tooling ====

Several tools already flag incorrect casing. Once PHP itself emits ''E_DEPRECATED'', teams have a runtime-backed reason to treat these as CI failures rather than style nits.

**PHPStan** reports class, function, method, static method, interface, trait, and enum name case mismatches via error identifiers such as [[https://phpstan.org/error-identifiers/class.nameCase|class.nameCase]] and [[https://phpstan.org/error-identifiers/function.nameCase|function.nameCase]]. The relevant config options (''checkInternalClassCaseSensitivity'', ''checkFunctionNameCase'') are off by default but enabled by the [[https://github.com/phpstan/phpstan-strict-rules|phpstan/phpstan-strict-rules]] extension.

**PHP-CS-Fixer** has two dedicated fixers: [[https://cs.symfony.com/doc/rules/casing/native_function_casing.html|native_function_casing]] and [[https://cs.symfony.com/doc/rules/casing/class_reference_name_casing.html|class_reference_name_casing]]. Both are included in the ''@PhpCsFixer'' and ''@Symfony'' rulesets.

**PhpStorm** has a built-in inspection [[https://www.jetbrains.com/help/inspectopedia/PhpMethodOrClassCallIsNotCaseSensitiveInspection.html|PhpMethodOrClassCallIsNotCaseSensitiveInspection]] that fires on functions, methods, classes, and namespaces called with different casing than their declarations. It is enabled by default.

**Psalm** does not yet implement case-sensitivity detection natively ([[https://github.com/vimeo/psalm/issues/1174|issue #1174]] from 2019 remains open). A sample plugin is available in the Psalm repository for teams that need it sooner.

**PHPCS** enforces lowercase PHP keywords via [[https://github.com/squizlabs/PHP_CodeSniffer/blob/master/src/Standards/Generic/Sniffs/PHP/LowerCaseKeywordSniff.php|Generic.PHP.LowerCaseKeyword]] but has no sniff for user-defined identifier casing.

Modern frameworks already enforce consistent casing. Well-maintained libraries and extensions are unaffected. Warnings appear in error logs as usual and behavior is identical across all SAPIs (CLI, CGI, FPM, embedded).

==== 5.3 Opcache and compile-time checks ====

A small number of deprecations fire at **compile time** rather than at runtime:

  * **Magic method declarations** (section 2.32): the check runs in ''zend_begin_method_decl()'' during compilation.
  * **Compile-time-bound function calls** (section 2.1, partial): when a function is known at compile time and resolved to ''ZEND_INIT_FCALL'', the case check runs during the compilation pass.

Under opcache, compiled bytecode is cached after the first compilation. These compile-time checks therefore fire **once at opcache warmup** (or on every page load without opcache) and are never re-triggered from the cached bytecode. In practice this is the desired behavior: the wrong-case declaration is in source code that the developer writes, and the warning surfaces immediately when the file is compiled — on the developer's machine, in CI, or at opcache preload time.

All other checks (runtime VM opcodes, ''zend_do_link_class'' for parent/trait/interface/namespace, built-in function calls) fire at runtime and are visible under opcache on the first execution of each affected call site.

==== 5.2 Performance ====

PHP currently lowercases function, method, class, and namespace names at every call site before the hash-table lookup. Once case-insensitive lookup is removed in PHP 9.0, that ''tolower'' step disappears from the hot path entirely. The gain per call is small, but it applies to every function, method, and namespaced class reference in every request.

TODO: performance check for Symfony benchmark

----

===== 6. Open Issues =====

None at this time. Edge cases are covered by 50 ''.phpt'' test files in ''tests/lang/case_sensitivity/''.

----

===== 7. Future Scope =====

==== 7.1 Potential future RFCs (not part of this proposal) ====

  * Make case-insensitivity an error in PHP 9.0 (separate RFC)

==== 7.2 Out of scope for this RFC ====

  * Changing built-in constant casing
  * Changing reserved keyword casing

----

===== 8. Voting Choices =====

Simple yes/no vote:

> "Do you approve of deprecating case-insensitive function, method, and class references in PHP 8.6, with the intent to enforce strict casing in PHP 9.0?"

Required: 2/3 majority (per PHP voting policy)

  * **Yes:** Proceed with deprecation as specified
  * **No:** Reject the RFC, keep case-insensitive behavior

----

===== 9. Implementation =====

Implementation is in progress.

----

===== 10. References =====

  * [[https://wiki.php.net/rfc/declare_strict_types|PHP RFC: Strict Types]]
  * [[https://www.php-fig.org/psr/psr-12/|PSR-12: Extended Coding Style Guide]]

----

===== 11. Prior art and previous discussions =====

Case sensitivity has come up on php-internals repeatedly since 2003. None of those discussions produced a merged RFC for functions or classes. What they did produce is a clear record of which arguments have been tried, which got traction, and which constraints keep resurfacing.

==== 11.1 PHP Bug #26575 — "Case Sensitive Class Names" (2003) ====

The earliest recorded request. Closed as Won't Fix by Andrey Zmievski: "This will break enormous number of applications." ([[https://bugs.php.net/bug.php?id=26575|bugs.php.net/bug.php?id=26575]])

==== 11.2 "Complete case-sensitivity in PHP" — php-internals, April 2012 ====

The first serious mailing-list thread. ([[https://externals.io/message/60228|externals.io/message/60228]])

Key participants: Nikita Popov, Matthew Weier O'Phinney, Yasuo Ohgaki, Galen Wright-Watson.

The thread established several points that still hold: IDE "Go to definition" reliability depends on case sensitivity, Python and Ruby are the correct peer comparisons, and the underlying problem is the ''tolower_map'' in ''zend_operators.c'' which is ASCII-only and would need to be removed rather than extended. Matthew Weier O'Phinney specifically cautioned against runtime ini options that change identifier resolution behavior.

==== 11.3 PHP Bug #62655 — "Request: optional class name case-sensitivity" (2012) ====

A follow-up bug proposing a three-level ''php.ini'' option. Suspended in April 2020 by cmb@php.net as "controversial, requiring internals list discussion." ([[https://bugs.php.net/bug.php?id=62655|bugs.php.net/bug.php?id=62655]])

==== 11.4 "PHP and case-sensitivity inconsistency" — php-internals, January 2014 ====

([[https://externals.io/message/71592|externals.io/message/71592]])

Stas Malyshev made the most durable argument from this period: any change must be all-or-nothing. A partial change — deprecating class names but not functions, or vice versa — creates a worse inconsistency than the current uniform behavior. Unicode case-folding was also raised: the ''tolower_map'' is ASCII only, and identifiers using Cyrillic or other scripts behave unpredictably. A ''use strict'' pragma modeled on JavaScript ES5 was proposed as a migration-friendly path; it never advanced.

==== 11.5 RFC: "Make the PHP core case-sensitive" — François Laupretre (2014) ====

([[https://wiki.php.net/rfc/case-sensitivity|wiki.php.net/rfc/case-sensitivity]])

The only RFC draft ever written on this topic, other than the present one. Proposed making namespaces, classes, interfaces, traits, functions, and non-magic methods all case-sensitive, with keywords becoming lowercase-only. Left incomplete, never voted on, status: Inactive.

==== 11.6 "Proposal for PHP 7: case-sensitive symbols" — php-internals, December 2014 ====

([[https://externals.io/message/79824|externals.io/message/79824]])

Key participants: François Laupretre (same author as the RFC above), Andrea Faulds, Marco Pivetta, Pierre Joye, Ferenc Kovacs.

Proposed pairing the PHP 7 release with case-sensitivity enforcement (E_STRICT in 7.x, removal in 8.0). The community reaction was mixed: PSR-4 and Linux portability were cited in favor; BC cost and ecosystem disruption dominated the opposition. No change was made in PHP 7.

==== 11.7 RFC: "Deprecate and Remove Case-Insensitive Constants" — Nikita Popov (2018) ====

([[https://wiki.php.net/rfc/case_insensitive_constant_deprecation|wiki.php.net/rfc/case_insensitive_constant_deprecation]])

The only successful related RFC. Targeted only the ''define()'' third-parameter ''$case_insensitive'' flag. The RFC text explicitly stated: "This is not an attempt to change case sensitivity for other identifiers (functions, classes, etc)." Nikita actively narrowed the discussion to constants when broader changes were proposed in replies. Passed; implemented as PHP 7.3 deprecation and PHP 8.0 removal.

The matching internals thread is at [[https://externals.io/message/100535|externals.io/message/100535]] (September 2017, Christoph Becker's initial proposal) and [[https://externals.io/message/102389|externals.io/message/102389]] (June 2018, Nikita's RFC announcement). Sara Golemon supported deprecating constants but said extending the change to functions and classes "would be a much more aggressive movement."

==== 11.8 "Revisiting case-sensitivity in PHP" — php-internals, June 2024 ====

([[https://externals.io/message/123573|externals.io/message/123573]])

Key participants: Valentin Udaltsov (initiator), Ben Ramsey, Levi Morrison, Gina P. Banyard, Timo Tijhof (Wikimedia).

Udaltsov noted that modern PSR-4 projects and static analyzers already enforce correct casing, so the practical migration burden has shrunk since 2014. Levi Morrison stated PHP 9.0 is the right enforcement target: "This isn't some minor thing squirreled away in a library — this is the core language." Gina P. Banyard raised namespace canonicalization (the class table stores lowercased keys; retrieving the canonical-cased name for the deprecation message requires reading ''ce->name'' rather than the table key).

Two points bear directly on this RFC:

  - **Serialized PHP 4 data.** Timo Tijhof noted that Wikimedia stores PHP 4-era serialized data where class names were written in lowercase. ''class_alias()'' was suggested as a migration path for applications in that situation.
  - **Nikita Popov's earlier private recommendation.** Udaltsov cited Nikita's suggestion to start with "class and class-like names first, then extend to functions" — the same ordering this RFC follows.

No RFC was drafted from this thread. The informal consensus was: PHP 9.0 enforcement, bundling namespaces + classes + functions in a single change rather than staggering them.

==== 11.9 Pull request record ====

Pull requests against [[https://github.com/php/php-src|php/php-src]] that touch this area. No PR targeting function, method, or class name case has ever been merged; the merged entries all relate to constants.

=== PR #965 — "Output an E_STRICT error on class/function case mismatch" (2014) ===

flaupretre (François Laupretre) · Closed, never merged · [[https://github.com/php/php-src/pull/965]]

A prototype that raised ''E_STRICT'' when a function or class is called with a name that does not match the declaration casing. Explicitly described as "a first step towards deprecating case-insensitive matches for functions and classes (and namespaces)." It was incomplete and closed without review consensus. This is the only prior PR that directly targets function and class name case sensitivity.

=== PR #3321 — "Deprecate case insensitive constants" (2018) ===

nikic (Nikita Popov) · Closed, superseded by a direct commit · [[https://github.com/php/php-src/pull/3321]]

Implements the [[https://wiki.php.net/rfc/case_insensitive_constant_deprecation|accepted RFC]] deprecating the ''$case_insensitive'' parameter of ''define()''. Scoped strictly to constants; the PR description explicitly excludes functions and classes. Shipped in PHP 7.3 via a separate commit after RFC vote passed.

=== PR #3770 — "[WIP] Remove deprecated functionality in PHP 8" (2019) ===

nikic · Closed WIP, never merged as-is · [[https://github.com/php/php-src/pull/3770]]

A broad work-in-progress collecting all deprecated features scheduled for removal in PHP 8, including case-insensitive constants. Relevant as context for how the constants cleanup was eventually landed in PHP 8.0.

=== PR #3833 — "Deprecate case-insensitive constants via typelib import" (2019) ===

cmb69 (Christoph M. Becker) · Closed, landed via a different commit · [[https://github.com/php/php-src/pull/3833]]

Extended the PHP 7.3 constant deprecation to the ''com_load_typelib()'' parameter and the ''com.autoregister_casesensitive'' INI setting in ''ext/com_dotnet'', which allowed case-insensitive constant imports from Windows typelibs.

=== PR #3836 — "Remove ability to import case-insensitive constants from typelibs" (2019) ===

cmb69 · Closed · [[https://github.com/php/php-src/pull/3836]]

Companion to #3833 proposing full removal of the ''$case_sensitive'' parameter and INI setting after the behavior was deprecated.

=== PR #9439 — "Do not generate CONST_CS when registering constants" (2022) ===

kocsismate · Merged · [[https://github.com/php/php-src/pull/9439]]

Stopped generating the now-meaningless ''CONST_CS'' flag in stubs when registering constants in extensions. Code-quality cleanup following PHP 8.0 removal.

=== PR #9685 — "Remove unnecessary usage of CONST_CS" (2022) ===

jorgsowa (Jorg Adam Sowa) · Merged · [[https://github.com/php/php-src/pull/9685]]

Cleaned up remaining ''CONST_CS'' flag usage throughout the codebase after case-insensitive constants were removed in PHP 8.0. A follow-up housekeeping PR to the PHP 8 removal.

=== PR #17071 — "Remove unused CONST_CS flag" (2024) ===

DanielEScherzer · Open · [[https://github.com/php/php-src/pull/17071]]

Proposes fully removing the ''CONST_CS'' flag from the API, since it has been unused since PHP 8.0. Still open at the time of writing.

==== 11.10 Summary ====

^ Year ^ Type ^ Event ^ Outcome ^
| 2003 | Bug | [[https://bugs.php.net/bug.php?id=26575|#26575]]: request for case-sensitive classes | Won't Fix |
| 2012 | Thread | [[https://externals.io/message/60228|"Complete case-sensitivity in PHP"]] | No change |
| 2012 | Bug | [[https://bugs.php.net/bug.php?id=62655|#62655]]: optional ini-level case-sensitivity | Suspended |
| 2014 | Thread | [[https://externals.io/message/71592|"PHP and case-sensitivity inconsistency"]] | No change |
| 2014 | RFC | [[https://wiki.php.net/rfc/case-sensitivity|"Make the PHP core case-sensitive"]] | Abandoned, never voted |
| 2014 | Thread | [[https://externals.io/message/79824|"Proposal for PHP 7: case-sensitive symbols"]] | No change in PHP 7 |
| 2014 | PR | [[https://github.com/php/php-src/pull/965|#965]]: E_STRICT on class/function case mismatch | Closed, never merged |
| 2018 | RFC | [[https://wiki.php.net/rfc/case_insensitive_constant_deprecation|Deprecate case-insensitive constants]] | **Accepted** — PHP 7.3 / 8.0 |
| 2018 | PR | [[https://github.com/php/php-src/pull/3321|#3321]]: deprecate case-insensitive constants | Closed (shipped via commit) |
| 2019 | PR | [[https://github.com/php/php-src/pull/3770|#3770]]: WIP remove PHP 8 deprecated features | Closed WIP |
| 2019 | PR | [[https://github.com/php/php-src/pull/3833|#3833]]: deprecate typelib case-insensitive constants | Closed (merged separately) |
| 2019 | PR | [[https://github.com/php/php-src/pull/3836|#3836]]: remove typelib case-insensitive constants | Closed |
| 2022 | PR | [[https://github.com/php/php-src/pull/9439|#9439]]: stop generating CONST_CS in stubs | Merged |
| 2022 | PR | [[https://github.com/php/php-src/pull/9685|#9685]]: remove CONST_CS usages | Merged |
| 2024 | Thread | [[https://externals.io/message/123573|"Revisiting case-sensitivity in PHP"]] | No RFC drafted |
| 2024 | PR | [[https://github.com/php/php-src/pull/17071|#17071]]: remove unused CONST_CS flag | Open |
| 2026 | RFC | This RFC | PHP 8.6 deprecation target |

PR #965 (2014) is the only prior PR that ever targeted function and class name case sensitivity. It was never merged. Every other merged entry relates to constants.

----

===== 12. Rejected Alternatives =====

==== 12.1 Make it an error immediately ====

Skipping a deprecation period and going straight to an error would break existing code without warning, which violates PHP's deprecation policy.
