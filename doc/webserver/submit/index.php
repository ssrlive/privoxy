<?php
//  File :  $Source: /cvsroot/ijbswa/current/doc/webserver/index.html,v $
//
//  Purpose  :  Submit form for ads and such
//              This file belongs in
//              ijbswa.sourceforge.net:/home/groups/i/ij/ijbswa/htdocs/
//
//  $Id: index.html,v 1.14 2002/03/27 09:34:26 swa Exp $
//
//  Written by and Copyright (C) 2001 the SourceForge
//  Privoxy team. http://www.privoxy.org/
//
//  Based on the Internet Junkbuster originally written
//  by and Copyright (C) 1997 Anonymous Coders and
//  Junkbusters Corporation.  http://www.junkbusters.com
//
//  This program is free software; you can redistribute it
//  and/or modify it under the terms of the GNU General
//  Public License as published by the Free Software
//  Foundation; either version 2 of the License, or (at
//  your option) any later version.
//
//  This program is distributed in the hope that it will
//  be useful, but WITHOUT ANY WARRANTY; without even the
//  implied warranty of MERCHANTABILITY or FITNESS FOR A
//  PARTICULAR PURPOSE.  See the GNU General Public
//  License for more details.
//
//  The GNU General Public License should be included with
//  this file.  If not, you can view it at
//  http://www.gnu.org/copyleft/gpl.html
//  or write to the Free Software Foundation, Inc., 59
//  Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//
$headers = getallheaders();
?>
<html>
  <head>
    <title>Privoxy|Submit</title>
    <link rel="stylesheet" type="text/css" href="../p_web.css">
</head>

<h1>Feedback</h1>

<p>Bla bla bla</p>

<h2>Advertisements</h2>

<p>You have seen an ad and want to support us making it disappear.
Great. Please fill out the form below and hit "Submit".</p>

<p>
<pre>
Privoxy Version: prefilled (not changeable)
Action file: prefilled (not changeable)
Action file release: prefilled with cvs-id-tag (not changeable)
The webpage where I observed the ad: prefilled with referrer (changeable)
????: ????
[...]
Remarks: (changeable)
Submit Button.


Submit button -> will present a second page, where verification 
takes place (correct version, latest blocklist, etc.). hence
the user has to confirm his submission on the second page.
</pre>
</p>

</body>
</html>
