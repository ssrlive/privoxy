<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">

<!--

  File        :  $Source: /cvsroot/ijbswa/current/doc/webserver/config/index.php,v $

  Purpose     :  Warn user of incorrect configuration.
                 This file belongs in
                 ijbswa.sourceforge.net:/home/groups/i/ij/ijbswa/htdocs/

  $Id: index.php,v 1.4.2.3 2003/10/16 13:40:52 oes Exp $

  Written by and Copyright (C) 2001 the SourceForge
  Privoxy team. http://www.privoxy.org/

  Based on the Internet Junkbuster originally written
  by and Copyright (C) 1997 Anonymous Coders and
  Junkbusters Corporation.  http://www.junkbusters.com

  This program is free software; you can redistribute it
  and/or modify it under the terms of the GNU General
  Public License as published by the Free Software
  Foundation; either version 2 of the License, or (at
  your option) any later version.

  This program is distributed in the hope that it will
  be useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.

  The GNU General Public License should be included with
  this file.  If not, you can view it at
  http://www.gnu.org/copyleft/gpl.html
  or write to the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston, MA  02111-1307, USA.

-->

<html>
<head>
  <title>Privoxy is not being used</title>
  <link rel="stylesheet" type="text/css" href="../p_web.css">
  <meta http-equiv="cache-control" content="no-cache">
  <meta http-equiv="expires" content="0">
</head>

<body>

<h1><a href="http://privoxy.org/">Privoxy</a> is not being used</h1>

<p>The fact that you are reading this page shows that Privoxy was
not used in the process of accessing it. Had the request been
made through Privoxy, it would have been intercepted and you
would be looking at Privoxy's web-based user interface now.</p>

<p><b>So what went wrong?</b> Chances are (in this order) that:</p>

<ul>
<li><p>this page is in your browser's cache. You've once been here
before starting to use Privoxy, and now your browser thinks that
it already knows the content of this page. Hence it doesn't request
a fresh copy.</p>
<p>Force your browser to do that. With most browsers, clicking "reload"
while holding down the shift key (shift-reloading) should suffice, but
you might need to manually clear the browser's cache (both memory and
disk cache).</p>

<li><p>your browser is not set up to use Privoxy.</p>
<p> Check your browser's proxy settings and make sure that it uses
127.0.0.1, port 8118 (or, if you did a custom configuration, whatever
different values you used).</p></li>

<li><p>when using multiple proxies in a chain, that either the chain
is broken at some point before Privoxy, or that an earlier proxy 
serves this page from its cache.</p>
<p>Shift-reload, clear all caches, and if the problem still persists,
trace the proxy chain starting with your browser's settings. Please
refer to the
<a href="http://www.privoxy.org/user-manual/config.html#FORWARDING">forwarding
chapter</a> of the <a href="http://www.privoxy.org/user-manual/">user
manual</a> for details.</p></li>
</ul>

<p><small>Until version 2.9.13, Privoxy was also known as Internet
Junkbuster. If you recently upgraded, then the web-based interface has
moved - it is now at <a
href="http://config.privoxy.org/">http://config.privoxy.org/</a>
(Short form: <a href="http://ijbswa.sourceforge.net/config/">p.p</a>
[Privoxy Proxy]).</small></p>

<p>If you have read the <a href="http://www.privoxy.org/user-manual/">user
manual</a> and still have trouble, feel free to <a
href="http://sourceforge.net/tracker/?group_id=11118&atid=211118">submit
a support request</a> to get help.</p>

</body>
</html>
