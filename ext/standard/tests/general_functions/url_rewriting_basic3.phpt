--TEST--
Test session URL-Rewriting with and without nested output_add_rewrite_var()
--EXTENSIONS--
session
--INI--
session.trans_sid_tags="a=href,area=href,frame=src,form="
url_rewriter.tags="a=href,area=href,frame=src,form="
--FILE--
<?php
$testTags = <<<TEST

<a href=""></a>
<a href="./foo.php"></a>

<a href="//php.net/foo.php"></a>
<a href="http://php.net/foo.php"></a>
<a href="bad://php.net/foo.php"></a>
<a href="//www.php.net/foo.php"></a>

<a href="//session-trans-sid.com/foo.php"></a>
<a href="http://session-trans-sid.com/foo.php"></a>
<a href="bad://session-trans-sid.com/foo.php"></a>
<a href="//www.session-trans-sid.com/foo.php"></a>

<a href="//url-rewriter.com/foo.php"></a>
<a href="http://url-rewriter.com/foo.php"></a>
<a href="bad://url-rewriter.com/foo.php"></a>
<a href="//www.url-rewriter.com/foo.php"></a>

<form action="" method="get"> </form>
<form action="./foo.php" method="get"></form>

<form action="//php.net/foo.php" method="get"></form>
<form action="http://php.net/foo.php" method="get"></form>
<form action="bad://php.net/foo.php" method="get"></form>
<form action="//www.php.net/foo.php" method="get"></form>

<form action="//session-trans-sid.com/bar.php" method="get"></form>
<form action="http://session-trans-sid.com/bar.php" method="get"></form>
<form action="bad://session-trans-sid.com/bar.php" method="get"></form>
<form action="//www.session-trans-sid.com/bar.php" method="get"></form>

<form action="//url-rewriter.com/bar.php" method="get"></form>
<form action="http://url-rewriter.com/bar.php" method="get"></form>
<form action="bad://url-rewriter.com/bar.php" method="get"></form>
<form action="//www.url-rewriter.com/bar.php" method="get"></form>

TEST;

ob_start();

ini_set('session.trans_sid_hosts', 'session-trans-sid.com');
ini_set('url_rewriter.hosts', 'url-rewriter.com');

ini_set('session.use_only_cookies', 0);
ini_set('session.use_cookies', 0);
ini_set('session.use_strict_mode', 0);
ini_set('session.use_trans_sid', 1);

session_id('testid');
session_start();

echo "URL-Rewriting with transparent session id support without output_add_rewrite_var()\n";
echo $testTags;

ob_flush();


output_add_rewrite_var('<name>', '<value>');

echo "\nURL-Rewriting with transparent session id support and output_add_rewrite_var()\n";
echo $testTags;

ob_end_flush();
output_reset_rewrite_vars();


output_add_rewrite_var('<name2>', '<value2>');

echo "\nURL-Rewriting with transparent session id support without output_add_rewrite_var()\n";
echo $testTags;

--EXPECTF--
Deprecated: ini_set(): Usage of session.trans_sid_hosts INI setting is deprecated in %s on line 44

Deprecated: ini_set(): Disabling session.use_only_cookies INI setting is deprecated in %s on line 47

Deprecated: ini_set(): Enabling session.use_trans_sid INI setting is deprecated in %s on line 50
URL-Rewriting with transparent session id support without output_add_rewrite_var()

<a href="?PHPSESSID=testid"></a>
<a href="./foo.php?PHPSESSID=testid"></a>

<a href="//php.net/foo.php"></a>
<a href="http://php.net/foo.php"></a>
<a href="bad://php.net/foo.php"></a>
<a href="//www.php.net/foo.php"></a>

<a href="//session-trans-sid.com/foo.php?PHPSESSID=testid"></a>
<a href="http://session-trans-sid.com/foo.php?PHPSESSID=testid"></a>
<a href="bad://session-trans-sid.com/foo.php"></a>
<a href="//www.session-trans-sid.com/foo.php"></a>

<a href="//url-rewriter.com/foo.php"></a>
<a href="http://url-rewriter.com/foo.php"></a>
<a href="bad://url-rewriter.com/foo.php"></a>
<a href="//www.url-rewriter.com/foo.php"></a>

<form action="" method="get"><input type="hidden" name="PHPSESSID" value="testid" /> </form>
<form action="./foo.php" method="get"><input type="hidden" name="PHPSESSID" value="testid" /></form>

<form action="//php.net/foo.php" method="get"></form>
<form action="http://php.net/foo.php" method="get"></form>
<form action="bad://php.net/foo.php" method="get"></form>
<form action="//www.php.net/foo.php" method="get"></form>

<form action="//session-trans-sid.com/bar.php" method="get"><input type="hidden" name="PHPSESSID" value="testid" /></form>
<form action="http://session-trans-sid.com/bar.php" method="get"><input type="hidden" name="PHPSESSID" value="testid" /></form>
<form action="bad://session-trans-sid.com/bar.php" method="get"></form>
<form action="//www.session-trans-sid.com/bar.php" method="get"></form>

<form action="//url-rewriter.com/bar.php" method="get"></form>
<form action="http://url-rewriter.com/bar.php" method="get"></form>
<form action="bad://url-rewriter.com/bar.php" method="get"></form>
<form action="//www.url-rewriter.com/bar.php" method="get"></form>

URL-Rewriting with transparent session id support and output_add_rewrite_var()

<a href="?%3Cname%3E=%3Cvalue%3E&PHPSESSID=testid"></a>
<a href="./foo.php?%3Cname%3E=%3Cvalue%3E&PHPSESSID=testid"></a>

<a href="//php.net/foo.php"></a>
<a href="http://php.net/foo.php"></a>
<a href="bad://php.net/foo.php"></a>
<a href="//www.php.net/foo.php"></a>

<a href="//session-trans-sid.com/foo.php?PHPSESSID=testid"></a>
<a href="http://session-trans-sid.com/foo.php?PHPSESSID=testid"></a>
<a href="bad://session-trans-sid.com/foo.php"></a>
<a href="//www.session-trans-sid.com/foo.php"></a>

<a href="//url-rewriter.com/foo.php?%3Cname%3E=%3Cvalue%3E"></a>
<a href="http://url-rewriter.com/foo.php?%3Cname%3E=%3Cvalue%3E"></a>
<a href="bad://url-rewriter.com/foo.php"></a>
<a href="//www.url-rewriter.com/foo.php"></a>

<form action="" method="get"><input type="hidden" name="PHPSESSID" value="testid" /><input type="hidden" name="&lt;name&gt;" value="&lt;value&gt;" /> </form>
<form action="./foo.php" method="get"><input type="hidden" name="PHPSESSID" value="testid" /><input type="hidden" name="&lt;name&gt;" value="&lt;value&gt;" /></form>

<form action="//php.net/foo.php" method="get"></form>
<form action="http://php.net/foo.php" method="get"></form>
<form action="bad://php.net/foo.php" method="get"></form>
<form action="//www.php.net/foo.php" method="get"></form>

<form action="//session-trans-sid.com/bar.php" method="get"><input type="hidden" name="PHPSESSID" value="testid" /></form>
<form action="http://session-trans-sid.com/bar.php" method="get"><input type="hidden" name="PHPSESSID" value="testid" /></form>
<form action="bad://session-trans-sid.com/bar.php" method="get"></form>
<form action="//www.session-trans-sid.com/bar.php" method="get"></form>

<form action="//url-rewriter.com/bar.php" method="get"><input type="hidden" name="&lt;name&gt;" value="&lt;value&gt;" /></form>
<form action="http://url-rewriter.com/bar.php" method="get"><input type="hidden" name="&lt;name&gt;" value="&lt;value&gt;" /></form>
<form action="bad://url-rewriter.com/bar.php" method="get"></form>
<form action="//www.url-rewriter.com/bar.php" method="get"></form>

URL-Rewriting with transparent session id support without output_add_rewrite_var()

<a href="?PHPSESSID=testid"></a>
<a href="./foo.php?PHPSESSID=testid"></a>

<a href="//php.net/foo.php"></a>
<a href="http://php.net/foo.php"></a>
<a href="bad://php.net/foo.php"></a>
<a href="//www.php.net/foo.php"></a>

<a href="//session-trans-sid.com/foo.php?PHPSESSID=testid"></a>
<a href="http://session-trans-sid.com/foo.php?PHPSESSID=testid"></a>
<a href="bad://session-trans-sid.com/foo.php"></a>
<a href="//www.session-trans-sid.com/foo.php"></a>

<a href="//url-rewriter.com/foo.php"></a>
<a href="http://url-rewriter.com/foo.php"></a>
<a href="bad://url-rewriter.com/foo.php"></a>
<a href="//www.url-rewriter.com/foo.php"></a>

<form action="" method="get"><input type="hidden" name="PHPSESSID" value="testid" /> </form>
<form action="./foo.php" method="get"><input type="hidden" name="PHPSESSID" value="testid" /></form>

<form action="//php.net/foo.php" method="get"></form>
<form action="http://php.net/foo.php" method="get"></form>
<form action="bad://php.net/foo.php" method="get"></form>
<form action="//www.php.net/foo.php" method="get"></form>

<form action="//session-trans-sid.com/bar.php" method="get"><input type="hidden" name="PHPSESSID" value="testid" /></form>
<form action="http://session-trans-sid.com/bar.php" method="get"><input type="hidden" name="PHPSESSID" value="testid" /></form>
<form action="bad://session-trans-sid.com/bar.php" method="get"></form>
<form action="//www.session-trans-sid.com/bar.php" method="get"></form>

<form action="//url-rewriter.com/bar.php" method="get"></form>
<form action="http://url-rewriter.com/bar.php" method="get"></form>
<form action="bad://url-rewriter.com/bar.php" method="get"></form>
<form action="//www.url-rewriter.com/bar.php" method="get"></form>
